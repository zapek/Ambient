/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2013 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: sysinfowinclass.c,v 1.27 2025/09/09 12:46:46 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <exec/system.h>
#include <libraries/sensors.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/sensors.h>
#include <proto/utility.h>
#include <string.h>

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "qport.h"
#include "screen.h"
#include "ambient_altivec.h"
#include "cpu.h"
#include "sound.h"
#include "prefs_advanced.h"

#define MKMHZ(x) (( (x) + 500000UL ) / 1000000UL)

APTR systeminfowin;

static LONG lastpage;
struct Library *SensorsBase; // bitRocky: already defined in proto/sensors.h without "static" so I removed "static"

#define	MAXCPU	4

struct Data {
	struct MUI_InputHandlerNode updatenode;
	APTR gauge_cpu_load;
	APTR cpu_graph;
	TEXT cpuver[MAXCPU][20];
	TEXT cpuload[40];

	// Memory
	APTR gauge_memory;
	ULONG memory_total;
	TEXT memtext[40];

	// Sensors
	APTR gauge_battery;
	APTR battery_sensor_notify;
	struct MsgPort SensorPort;
	struct MUI_InputHandlerNode sensornode;
};

STATIC CONST struct TagItem battery_sensortags[] =
{
	{ SENSORS_Type, SensorType_Battery },
	{ TAG_DONE, 0 }
};

STATIC ULONG GetBatteryCount(void)
{
	ULONG count = 0;

	if (SensorsBase)
	{
		count = GetSensorsCount((struct TagItem *)&battery_sensortags);
	}

	return count;
}

STATIC VOID AddSensorNotify(APTR obj, struct Data *data)
{
	if (SensorsBase)
	{
		APTR sensorlist, sensor;

		sensorlist = ObtainSensorsList((struct TagItem *)&battery_sensortags);
		sensor = NULL;

		while ((sensor = NextSensor(sensor, sensorlist, NULL)))
		{
			struct TagItem battags[5];
			ULONG capacity = 0, max_capacity = 0, charging = 0, present = 0;

			battags[0].ti_Tag  = SENSORS_Battery_Capacity;
			battags[0].ti_Data = (IPTR)&capacity;
			battags[1].ti_Tag  = SENSORS_Battery_MaxCapacity;
			battags[1].ti_Data = (IPTR)&max_capacity;
			battags[2].ti_Tag  = SENSORS_Battery_Charging;
			battags[2].ti_Data = (IPTR)&charging;
			battags[3].ti_Tag  = SENSORS_Battery_Present;
			battags[3].ti_Data = (IPTR)&present;
			battags[4].ti_Tag  = TAG_DONE;

			GetSensorAttr(sensor, battags);

			if (present)
			{
				SetAttrs(data->gauge_battery, MUIA_Gauge_Max, max_capacity, MUIA_Gauge_Current, capacity,
					MUIA_Gauge_InfoText, charging ? GSI(MSG_SYSINFOWIN_BATTERYCHARGING_INFO) : GSI(MSG_SYSINFOWIN_BATTERY_INFO),
					TAG_DONE);

				battags[0].ti_Tag  = SENSORS_Notification_Destination;
				battags[0].ti_Data = (IPTR)&data->SensorPort;
				battags[1].ti_Tag  = TAG_DONE;
			
				data->battery_sensor_notify = StartSensorNotify(sensor, battags);

				if (data->battery_sensor_notify)
				{
					data->sensornode.ihn_Object = obj;
					data->sensornode.ihn_Signals = 1 << data->SensorPort.mp_SigBit;
					data->sensornode.ihn_Method = MM_Sysinfowin_UpdateSensorData;
					DoMethod(app, MUIM_Application_AddInputHandler, &data->sensornode);
					break;
				}
			}
			else
			{
				set(data->gauge_battery, MUIA_Gauge_InfoText, GSI(MSG_SYSINFOWIN_BATTERY_MISSING));
			}
		}

		ReleaseSensorsList(sensorlist, NULL);
	}
}

#define systemmemfmtlen 64

DEFNEW
{
	struct Data *data;
	APTR mi_quit;
	APTR grp_reg;
	APTR system_group;
	APTR cpu_group;
	APTR txt_machine, txt_bustick, txt_cpucount;
	APTR txt_cpu[MAXCPU], txt_vec[MAXCPU];
	APTR txt_l1cachesize[MAXCPU], txt_l1flags[MAXCPU];
	APTR txt_l2cachesize[MAXCPU], txt_l2flags[MAXCPU];
	APTR txt_l3cachesize[MAXCPU], txt_l3flags[MAXCPU];
	APTR txt_sounddriver, gauge_cpu_load, cpu_graph, gauge_memory;
	APTR gauge_battery = NULL;
	TEXT t[256];
	TEXT txt_systemmemory[systemmemfmtlen];
	CONST_STRPTR cachesizefmt = GSI(MSG_SYSINFOWIN_CACHESIZE);
	const int cachesizefmtlen = strlen(cachesizefmt) + 1;
	CONST_STRPTR cachespecfmt = GSI(MSG_SYSINFOWIN_CACHESPECS);
	const int cachespecfmtlen = strlen(cachespecfmt) + 1;
	TEXT l1_size[cachesizefmtlen];
	TEXT l1_specs[cachespecfmtlen];
	TEXT l2_size[cachesizefmtlen];
	TEXT l2_specs[cachespecfmtlen];
	TEXT l3_size[cachesizefmtlen];
	TEXT l3_specs[cachespecfmtlen];
	ULONG	has_datastream, has_perfmon, battery_sensor_count;
	UQUAD tll;

	static STRPTR regtitle[3];
	static STRPTR unknown;

	regtitle[0] = GSI(MSG_SYSINFOWIN_MONITOR_REGTITLE);
	regtitle[1] = GSI(MSG_SYSINFOWIN_INFO_REGTITLE);
	unknown = GSI(MSG_UNKNOWN);

	{   /* system memory */
		FLOAT memorymib = AvailMem( MEMF_FAST | MEMF_TOTAL );
		FLOAT memorymb;
		char *memorys;

		if( memorymib >= ( 1024*1024*1024 ) ) {
			memorymb = ( memorymib / ( 1000 * 1000 * 1000 ) ) + 0.005f;
			memorymib  = ( memorymib / ( 1024 * 1024 * 1024 ) ) + 0.005f;
			memorys = "%0.2f GiB (%0.2f GB)";
		} else {
//			if ( memorymib >= ( 1024 * 1024 ) ) {
				memorymb = ( memorymib / ( 1000 * 1000 ) ) + 0.005f;
				memorymib  = ( memorymib / ( 1024 * 1024 ) ) + 0.005f;
				memorys = "%0.2f MiB (%0.2f MB)";
//			}
		}
		NewRawDoFmt( memorys, NULL, txt_systemmemory, memorymib, memorymb );
	}

	NewRawDoFmt(cachesizefmt, NULL, l1_size, "L1");
	NewRawDoFmt(cachespecfmt, NULL, l1_specs, "L1");
	NewRawDoFmt(cachesizefmt, NULL, l2_size, "L2");
	NewRawDoFmt(cachespecfmt, NULL, l2_specs, "L2");
	NewRawDoFmt(cachesizefmt, NULL, l3_size, "L3");
	NewRawDoFmt(cachespecfmt, NULL, l3_specs, "L3");

	/* We assume following data doesn't change... */

	NewGetSystemAttrs(&has_datastream, sizeof(has_datastream), SYSTEMINFOTYPE_PPC_DATASTREAM, TAG_DONE);
	NewGetSystemAttrs(&has_perfmon, sizeof(has_perfmon), SYSTEMINFOTYPE_PPC_PERFMONITOR, TAG_DONE);
	NewGetSystemAttrs(&tll, sizeof(tll), SYSTEMINFOTYPE_PPC_BUSCLOCK, TAG_DONE);
	NewRawDoFmt("%llu MHz", NULL, t, MKMHZ(tll));

	SensorsBase = OpenLibrary("sensors.library", 51);
	battery_sensor_count = GetBatteryCount();

	if (battery_sensor_count > 0)
	{
		gauge_battery = GaugeObject,
			GaugeFrame,
			MUIA_Gauge_Horiz, TRUE,
		End;
	}

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_SYSINFOWIN_TITLE),
		MUIA_Window_ID, MAKE_ID('S','Y','S','M'),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_Menustrip, MenustripObject,
			Child, MenuObject,
				MUIA_Menu_Title, GSI( MSG_SYSINFOWIN_MENU_PROJECT ),
				Child, mi_quit = MenuitemObject,
					MUIA_Menuitem_Title,    GSI( MSG_SYSINFOWIN_MENU_QUIT ),
					MUIA_Menuitem_Shortcut, "Q",
				End,
			End,
		End,

		WindowContents, VGroup,

			Child, grp_reg = RegisterGroup(regtitle),
				MUIA_CycleChain, TRUE,
				MUIA_Group_ActivePage, lastpage,

				Child, VGroup,
					Child, cpu_graph = NewObject(getgraphclass(), NULL, MA_Graph_ScaleValue, 1000, TAG_DONE),

					Child, gauge_cpu_load = GaugeObject,
						GaugeFrame,
						MUIA_Gauge_Horiz, TRUE,
						MUIA_Gauge_Max, 1000,
						MUIA_Gauge_InfoText, "-",
					End,

					Child, gauge_memory = GaugeObject,
						GaugeFrame,
						MUIA_Gauge_Horiz, TRUE,
						MUIA_Gauge_Max, 1000,
						MUIA_Gauge_InfoText, "-",
					End,

					gauge_battery ? Child : TAG_IGNORE, gauge_battery,
/*
					Child, ScaleObject,
					End,
*/
				End,

				Child, ScrollgroupObject,
					MUIA_Scrollgroup_AutoBars, TRUE, /* MUI V20 */

					MUIA_Scrollgroup_Contents, VGroupV,

						Child, VGroup, //GroupFrameT(GSI(MSG_SYSINFOWIN_SYSTEM_GROUP)),
							Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Rectangle_BarTitle, GSI(MSG_SYSINFOWIN_SYSTEM_GROUP), End,
							Child, HGroup,
								Child, HSpace(0),
								Child, system_group = ColGroup(2),

									Child, NSLabel2(MSG_SYSINFOWIN_MACHINE),
									Child, txt_machine = MUICreateTextNoFrame(MSG_SYSINFOWIN_MACHINE, unknown),

									Child, NSLabel2(MSG_SYSINFOWIN_BCLOCK),
									Child, MUICreateTextNoFrame(MSG_SYSINFOWIN_BCLOCK, t),

									Child, NSLabel2(MSG_SYSINFOWIN_TTICKS),
									Child, txt_bustick = MUICreateTextNoFrame(MSG_SYSINFOWIN_TTICKS, unknown),

									Child, NSLabel2(MSG_SYSINFOWIN_CPUCOUNT),
									Child, txt_cpucount = MUICreateTextNoFrame(MSG_SYSINFOWIN_CPUCOUNT, unknown),

									Child, NSLabel2(MSG_SYSINFOWIN_SYSTEMMEMORY),
									Child, MUICreateTextNoFrame(MSG_SYSINFOWIN_SYSTEMMEMORY, txt_systemmemory ),
								End,
								Child, HSpace(0),
							End,
						End,

						Child, cpu_group = VGroup,
							MUIA_Group_PageMax, FALSE,
							MUIA_Group_PageMode, TRUE,
						End,

						Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Rectangle_BarTitle, GSI(MSG_SYSINFOWIN_AMBIENT_GROUP), End,
						Child, HGroup, //GroupFrameT(GSI(MSG_SYSINFOWIN_AMBIENT_GROUP)),
							Child, HSpace(0),
							Child, ColGroup(2),

								Child, NSLabel2(MSG_SYSINFOWIN_SNDDRIVER),
								Child, txt_sounddriver = MUICreateTextNoFrame(MSG_SYSINFOWIN_SNDDRIVER, sound_getattr(SOUNDCONTEXT_DEFAULT, SOUNDATTR_DriverName)),
							End,
							Child, HSpace(0),
						End,
					End, /* VGroupV */

				End, /* ScrollGroup */
			End,
		End,
	End;

	if (obj)
	{
		Object *title_group=NULL;
		ULONG is_peg2, cpucount=1, cpuindex;
		STRPTR s;

		data = INST_DATA(cl, obj);

		CreateQPort(&data->SensorPort);

		if (gauge_battery)
		{
			data->gauge_battery = gauge_battery;
			AddSensorNotify(obj, data);
		}

		data->updatenode.ihn_Object = obj;
		data->updatenode.ihn_Millis = 1000;
		data->updatenode.ihn_Method = MM_Sysinfowin_UpdateSystemInfo;
		data->updatenode.ihn_Flags   = MUIIHNF_TIMER;

		data->gauge_cpu_load = gauge_cpu_load;
		data->cpu_graph = cpu_graph;

		data->gauge_memory = gauge_memory;
		data->memory_total = AvailMem(MEMF_FAST | MEMF_TOTAL);

		NewGetSystemAttrs(t, sizeof(t), SYSTEMINFOTYPE_SYSTEM, TAG_DONE);
		DoMethod(txt_machine, MUIM_SetAsString, MUIA_Text_Contents, "%s", t);

		is_peg2 = strcmp(t, "bplan,Pegasos2") ? FALSE : TRUE;

		NewGetSystemAttrs(&cpucount,  sizeof(cpucount),  SYSTEMINFOTYPE_CPUCOUNT,  TAG_DONE);

		DoMethod(txt_cpucount, MUIM_SetAsString, MUIA_Text_Contents, "%ld", cpucount);

		if (cpucount > 1)
		{
			DoMethod(cpu_group, OM_ADDMEMBER, title_group = MUI_NewObject(MUIC_Title,MUIA_CycleChain, 1, TAG_END),
					End;
		}

		for (cpuindex=0;cpuindex < min(MAXCPU,cpucount);cpuindex++)
		{
			ULONG cpuid, tl, tl1, tl2=0;
			if (title_group)
			{
				char buf[256];
				snprintf(buf,sizeof(buf),GSI(MSG_SYSINFOWIN_CPU_TITLE),cpuindex);
				DoMethod(title_group, OM_ADDMEMBER,
							VGroup,
								MUIA_Group_VertSpacing, 0,
								//Child, MUI_NewObject(MUIC_Dtpic,MUIA_Dtpic_Name, name, /*MUIA_Dtpic_Alpha, 10*255/100,*/ /*MUIA_Dtpic_MinWidth, 8,*/ /*MUIA_Image_FreeHoriz, TRUE,*/ TAG_DONE),
								Child, TextObject, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, buf, End,
							End,
					End;
			}


			DoMethod(cpu_group, OM_ADDMEMBER,
					VGroup,
						Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Rectangle_BarTitle, GSI(MSG_SYSINFOWIN_CPU_GROUP), End,
						Child, HGroup, //GroupFrameT(GSI(MSG_SYSINFOWIN_CPU_GROUP)),
							Child, HSpace(0),
							Child, ColGroup(2),

								Child, NSLabel2(MSG_SYSINFOWIN_MODEL),
								Child, txt_cpu[cpuindex] = MUICreateTextNoFrame(MSG_SYSINFOWIN_MODEL, unknown),

								Child, NSLabel2(MSG_SYSINFOWIN_ALTIVEC),
								Child, txt_vec[cpuindex] = MUICreateTextNoFrame(MSG_SYSINFOWIN_ALTIVEC, unknown),

								Child, NSLabel2(MSG_SYSINFOWIN_DATASTREAM),
								Child, MUICreateTextNoFrame(MSG_SYSINFOWIN_DATASTREAM, (has_datastream ? GSI(MSG_YES) : GSI(MSG_NO))),

								Child, NSLabel2(MSG_SYSINFOWIN_PMON),
								Child, MUICreateTextNoFrame(MSG_SYSINFOWIN_PMON, (has_perfmon ? GSI(MSG_YES) : GSI(MSG_NO))),
							End,
							Child, HSpace(0),
						End,

						Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Rectangle_BarTitle, GSI(MSG_SYSINFOWIN_CPUCACHE_GROUP), End,
						Child, HGroup, //GroupFrameT(GSI(MSG_SYSINFOWIN_CPUCACHE_GROUP)),
							Child, HSpace(0),
							Child, ColGroup(2),
								Child, Label2(l1_specs),
								Child, txt_l1flags[cpuindex] = NewObject(getaddtextclass(), NULL,
									MUIA_Text_Contents, unknown,
									MUIA_ShortHelp, GSI(MSG_SYSINFOWIN_CACHESPECS_HELP),
								End,

								Child, Label2(l2_specs),
								Child, txt_l2flags[cpuindex] = NewObject(getaddtextclass(), NULL,
									MUIA_Text_Contents, unknown,
									MUIA_ShortHelp, GSI(MSG_SYSINFOWIN_CACHESPECS_HELP),
								End,

								Child, Label2(l3_specs),
								Child, txt_l3flags[cpuindex] = NewObject(getaddtextclass(), NULL,
									MUIA_Text_Contents, unknown,
									MUIA_ShortHelp, GSI(MSG_SYSINFOWIN_CACHESPECS_HELP),
								End,

								Child, Label2(l1_size),
								Child, txt_l1cachesize[cpuindex] = MUICreateTextNoFrame(MSG_SYSINFOWIN_CACHESIZE_HELP-1, unknown),

								Child, Label2(l2_size),
								Child, txt_l2cachesize[cpuindex] = NewObject(getaddtextclass(), NULL,
									MUIA_Text_Contents, unknown,
									MUIA_ShortHelp, GSI(MSG_SYSINFOWIN_CACHESIZE_HELP),
								End,

								Child, Label2(l3_size),
								Child, txt_l3cachesize[cpuindex] = MUICreateTextNoFrame(MSG_SYSINFOWIN_CACHESIZE_HELP-1, unknown),
							End,
							Child, HSpace(0),
						End,
					End);

			/* CPU type and clock */

			NewGetSystemAttrs(&tl,  sizeof(tl),  SYSTEMINFOTYPE_PPC_CPUVERSION,  SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
			NewGetSystemAttrs(&tl1, sizeof(tl1), SYSTEMINFOTYPE_PPC_CPUREVISION, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
			NewGetSystemAttrs(&tll, sizeof(tll), SYSTEMINFOTYPE_PPC_CPUCLOCK,    SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);

			cpuid = cpu_id(tl, tl1);

			/*
			 * The Pegasos2 OF is bugged and recognizes
			 * a 750CXe as a 745/755.
			 */
			if (is_peg2 && (cpuid == CPUID_745_755))
			{
				cpuid = CPUID_750CXe;
			}

			{
				const char *cpuname;
				char namebuf[20];
				if (NewGetSystemAttrs(namebuf, sizeof(namebuf), SYSTEMINFOTYPE_CPUNAME, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE))
				{
					cpuname = namebuf;
				}
				else
				{
					cpuname = cpu_name(cpuid);
				}
				DoMethod(txt_cpu[cpuindex], MUIM_SetAsString, MUIA_Text_Contents, "%s/%lU MHz", cpuname, (ULONG)MKMHZ(tll));
			}
			snprintf(data->cpuver[cpuindex], sizeof(data->cpuver), "%04lx%04lx", tl, tl1);
			set(txt_cpu[cpuindex], MUIA_ShortHelp, data->cpuver);

			NewGetSystemAttrs(&tl, sizeof(tl), SYSTEMINFOTYPE_PPC_BUSTICKS, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
			DoMethod(txt_bustick, MUIM_SetAsString, MUIA_Text_Contents, "%lU", tl);

			set(txt_vec[cpuindex], MUIA_Text_Contents, has_altivec ? GSI(MSG_YES) : GSI(MSG_NO));

			/* L1 */
			NewGetSystemAttrs(&tl, sizeof(tl), SYSTEMINFOTYPE_PPC_ICACHEL1SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
			NewGetSystemAttrs(&tl1, sizeof(tl1), SYSTEMINFOTYPE_PPC_DCACHEL1SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
			DoMethod(txt_l1cachesize[cpuindex], MUIM_SetAsString, MUIA_Text_Contents, GSI(MSG_SYSINFOWIN_CACHESPECSDETAILS1_INFO), tl / 1024, tl1 / 1024);

			{
				NewGetSystemAttrs(&tl, sizeof(tl), SYSTEMINFOTYPE_PPC_CACHEL1FLAGS, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				set(txt_l1flags[cpuindex], MUIA_Text_Contents,"");
				cpu_cachebuf(txt_l1flags[cpuindex], tl);
			}

			/* L2 */
			tl = 0;
			NewGetSystemAttrs(&tl, sizeof(tl), SYSTEMINFOTYPE_PPC_ICACHEL2SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				
			if (tl)
			{
				/*
				 * The default Pegasos1 OF is bugged
				 * and reports 512kB for the 750CXe.
				 *
				 * The Pegasos2 OF is bugged and doesn't
				 * set the 750CXe cache size properly.
				 */
				if (cpu_flags(cpuid) & CPUF_L2_256)
				{
					if (tl != 262144)
					{
						tl = 262144;
					}
				}

				/*
				 * The default Pegasos2 OF is bugged
				 * and reports 256kB for the 7447.
				 */
				if (cpu_flags(cpuid) & CPUF_L2_512)
				{
					if (tl < 524288)
					{
						tl = 524288;
					}
				}

				/*
				 * Sigh.. bplan must really die..
				 * I should hardcode everything and
				 * never ask that fucking OF.
				 */

				NewGetSystemAttrs(&tl1, sizeof(tl1), SYSTEMINFOTYPE_PPC_CACHEL2FLAGS, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				NewGetSystemAttrs(&tl2, sizeof(tl2), SYSTEMINFOTYPE_PPC_DCACHEL2SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				DoMethod(txt_l2cachesize[cpuindex], MUIM_SetAsString, MUIA_Text_Contents, (tl1 & CACHEFLAGF_UNIFIED) ? GSI(MSG_SYSINFOWIN_CACHESPECSDETAILS2_INFO) : GSI(MSG_SYSINFOWIN_CACHESPECSDETAILS1_INFO), tl / 1024, tl2 / 1024);
				set(txt_l2flags[cpuindex], MUIA_Text_Contents,"");
				cpu_cachebuf(txt_l2flags[cpuindex], tl1);
			}
			else
			{
				set(txt_l2cachesize[cpuindex], MUIA_Text_Contents, "N/A");
				set(txt_l2flags[cpuindex], MUIA_Text_Contents, "N/A");
			}

			/* L3 */
			tl = 0;
			NewGetSystemAttrs(&tl, sizeof(tl), SYSTEMINFOTYPE_PPC_ICACHEL3SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				
			/*
			 * The default Pegasos2 OF is bugged
			 * and reports a L3 cache for the 7447.
			 */
			if (tl && (cpu_flags(cpuid) & CPUF_L3_NONE))
			{
				tl = 0;
			}

			if (tl)
			{
				NewGetSystemAttrs(&tl1, sizeof(tl1), SYSTEMINFOTYPE_PPC_CACHEL3FLAGS, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);
				NewGetSystemAttrs(&tl2, sizeof(tl2), SYSTEMINFOTYPE_PPC_DCACHEL3SIZE, SYSTEMINFOTAG_CPUINDEX, cpuindex, TAG_DONE);

				DoMethod(txt_l3cachesize[cpuindex], MUIM_SetAsString, MUIA_Text_Contents, (tl1 & CACHEFLAGF_UNIFIED) ? GSI(MSG_SYSINFOWIN_CACHESPECSDETAILS2_INFO) : GSI(MSG_SYSINFOWIN_CACHESPECSDETAILS1_INFO), tl / 1024, tl2 / 1024);
				set(txt_l3flags[cpuindex], MUIA_Text_Contents,"");
				cpu_cachebuf(txt_l3flags[cpuindex], tl1);
			}
			else
			{
				set(txt_l3cachesize[cpuindex], MUIA_Text_Contents, "N/A");
				set(txt_l3flags[cpuindex], MUIA_Text_Contents, "N/A");
			}
		}

		s = sound_getattr(SOUNDCONTEXT_DEFAULT, SOUNDATTR_DriverName);
		DoMethod(txt_sounddriver, MUIM_SetAsString, MUIA_Text_Contents, "%s", (ULONG)s ? (ULONG)s : (ULONG) GSI(MSG_SYSINFOWIN_SNDDRIVER_NONE_INFO));

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
			obj, 1, MM_Sysinfowin_Close
		);

/* menu item notifies */

		DoMethod( mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, MM_Sysinfowin_Close );
	}

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (data->battery_sensor_notify)
	{
		EndSensorNotify(data->battery_sensor_notify, NULL);
		DoMethod(app, MUIM_Application_RemInputHandler, &data->sensornode);
	}

	DeleteQPort(&data->SensorPort);
	CloseLibrary(SensorsBase);

	return (DOSUPER);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_SystemInfo;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFMMETHOD(Window_Setup)
{
	GETDATA;
	ULONG ok;

	ok = DOSUPER;

	if (ok)
	{
		DoMethod(app, MUIM_Application_AddInputHandler, &data->updatenode);
	}

	return ok;
}

DEFMMETHOD(Window_Cleanup)
{
	GETDATA;

	DoMethod(app, MUIM_Application_RemInputHandler, &data->updatenode);

	return (DOSUPER);
}

DEFTMETHOD(Sysinfowin_UpdateSensorData)
{
	GETDATA;

	struct SensorsNotificationMessage *smsg = (APTR)GetMsg(&data->SensorPort);

	if (smsg)
	{
		struct TagItem *tstate = smsg->Notifications, *tag;

		while ((tag = NextTagItem(&tstate)))
		{
			switch (tag->ti_Tag)
			{
				case SENSORS_Battery_Capacity:
					SetAttrs(data->gauge_battery, MUIA_Gauge_Current, tag->ti_Data, TAG_DONE);
					break;

				case SENSORS_Battery_Charging:
					SetAttrs(data->gauge_battery, MUIA_Gauge_InfoText, tag->ti_Data ? GSI(MSG_SYSINFOWIN_BATTERYCHARGING_INFO) : GSI(MSG_SYSINFOWIN_BATTERY_INFO), TAG_DONE);
					break;
			}
		}
	}

	return 0;
}

DEFTMETHOD(Sysinfowin_UpdateSystemInfo)
{
	GETDATA;

	UQUAD lastsecticks, lastseccputime;
	LONG time1, time2, time3;
	ULONG curr;
	static LONG otime;

	NewGetSystemAttrsA(&lastsecticks,   sizeof(lastsecticks),   SYSTEMINFOTYPE_LASTSECTICKS,   NULL);
	NewGetSystemAttrsA(&lastseccputime, sizeof(lastseccputime), SYSTEMINFOTYPE_LASTSECCPUTIME, NULL);

	time1 = lastseccputime * 10000 / lastsecticks;
	time2 = time1 % 100;
	time1 /= 100;

	if (time1 >= 100)
	{
		time1 = 100;
		time2 = 0;
	}
	else if (time2 < 0)
	{
		time2 = 0;
	}

	time3 = time1 * 10 + time2 / 10;

	/* We only update if we see a change... */
	if ((time1 + time2) != otime)
	{
		DoMethod(data->cpu_graph, MM_Graph_Add, (ULONG) time3);
		otime = time1 + time2;
	}

	snprintf(data->cpuload, sizeof(data->cpuload), "%s: %ld.%2.2ld%%%%", GSI(MSG_SYSINFOWIN_CPULOAD), time1, time2);
	SetAttrs(data->gauge_cpu_load, MUIA_Gauge_InfoText, data->cpuload, MUIA_Gauge_Current, time3, TAG_DONE);

	curr = AvailMem(MEMF_FAST);
	curr = (ULONG)((double)curr / (double)data->memory_total * 1000.0);

	snprintf(data->memtext, sizeof(data->memtext), GSI(MSG_SYSINFOWIN_MEMORY), (curr + 5) / 10);
	SetAttrs(data->gauge_memory, MUIA_Gauge_InfoText, data->memtext, MUIA_Gauge_Current, curr, TAG_DONE);

	return 0;
}


DEFTMETHOD(Sysinfowin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	systeminfowin = NULL;
	return (0);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(Window_Setup)
DECMMETHOD(Window_Cleanup)
DECTMETHOD(Sysinfowin_UpdateSensorData)
DECTMETHOD(Sysinfowin_UpdateSystemInfo)
DECTMETHOD(Sysinfowin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, sysinfowinclass)

