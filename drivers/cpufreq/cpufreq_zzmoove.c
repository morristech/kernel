/*
 *  drivers/cpufreq/cpufreq_zzmoove.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 * - ZZMoove Governor v0.6 by ZaneZam 2012/13 Changelog:                                                                                                                          -
 * --------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Version 0.1 - first release
 *
 *	- codebase latest smoove governor version from midnight kernel (https://github.com/mialwe/mngb/)
 *	- modified frequency tables to match I9300 standard frequency range 200-1400 mhz
 *	- added cpu hotplug functionality with strictly cpu switching
 *	  (modifications partially taken from ktoonservative governor from
 *	  ktoonsez KT747-JB kernel https://github.com/ktoonsez/KT747-JB)
 *
 * Version 0.2 - improved
 *
 *	- added tuneables to be able to adjust values on early suspend (screen off) via sysfs instead
 *	  of using only hardcoded defaults
 *	- modified hotplug implementation to be able to tune threshold range per core indepentently
 *	  and to be able to manually turn cores offline
 *
 *	  for this functions following new tuneables were indroduced:
 *
 *	  sampling_rate_sleep_multiplier -> sampling rate multiplier on early suspend (possible values 1 or 2, default: 2)
 *	  up_threshold_sleep		 -> up threshold on early suspend (possible range from above "down_threshold_sleep" up to 100, default: 90)
 *	  down_threshold_sleep		 -> down threshold on early suspend (possible range from 11 to "under up_threshold_sleep", default: 44)
 *	  smooth_up_sleep		 -> smooth up scaling on early suspend (possible range from 1 to 100, default: 100)
 *	  up_threshold_hotplug1		 -> hotplug threshold for cpu1 (0 disable core1, possible range from "down_threshold" up to 100, default: 68)
 *	  up_threshold_hotplug2		 -> hotplug threshold for cpu2 (0 disable core2, possible range from "down_threshold" up to 100, default: 68)
 *	  up_threshold_hotplug3		 -> hotplug threshold for cpu3 (0 disable core3, possible range from "down_threshold" up to 100, default: 68)
 *	  down_threshold_hotplug1	 -> hotplug threshold for cpu1 (possible range from 11 to under "up_threshold", default: 55)
 *	  down_threshold_hotplug2	 -> hotplug threshold for cpu2 (possible range from 11 to under "up_threshold", default: 55)
 *	  down_threshold_hotplug3	 -> hotplug threshold for cpu3 (possible range from 11 to under "up_threshold", default: 55)
 *
 * Version 0.3 - more improvements
 *
 *	- added tuneable "hotplug_sleep" to be able to turn cores offline only on early suspend (screen off) via sysfs
 *	  possible values: 0 do not touch hotplug-settings on early suspend, values 1, 2 or 3 are equivalent to
 *	  cores which should be online at early suspend
 *	- modified scaling frequency table to match "overclock" freqencies to max 1600 mhz
 *	- fixed black screen of dead problem in hotplug logic due to missing mutexing on 3-core and 2-core settings
 *	- code cleaning and documentation
 *
 * Version 0.4 - limits
 *
 *	- added "soft"-freqency-limit. the term "soft" means here that this is unfortuneately not a hard limit. a hard limit is only possible with 
 *	  cpufreq driver which is the freqency "giver" the governor is only the "consultant". So now the governor will scale up to only the given up 
 *	  limit on higher system load but if the cpufreq driver "wants" to go above that limit the freqency will go up there. u can see this for 
 *	  example with touchboost or wake up freqencies (1000 and 800 mhz) where the governor obviously will be "bypassed" by the cpufreq driver.
 *	  but nevertheless this soft-limit will now reduce the use of freqencies higer than given limit and therefore it will reduce power consumption.
 *
 *	  for this function following new tuneables were indroduced:
 *
 *	  freq_limit_sleep		 -> limit freqency on early suspend (possible values 0 disable limit, 200-1600, default: 0)
 *	  freq_limit			 -> limit freqency on awake (possible values 0 disable limit, 200-1600, default: 0)
 *
 *	- added scaling frequencies to frequency tables for a faster up/down scaling. This should bring more performance but on the other hand it
 *	  can be of course a little bit more power consumptive.
 *
 *	  for this function following new tuneables were indroduced:
 *	
 *	  fast_scaling			 -> fast scaling on awake (possible values 0 disable or 1 enable, default: 0)
 *	  fast_scaling_sleep (sysfs)	 -> fast scaling on early suspend (possible values 0 disable or 1 enable, default: 0)
 *
 *	- added tuneable "freq_step_sleep" for setting the freq step at early suspend (possible values same as freq_step 0 to 100, default 5)
 *	- added DEF_FREQ_STEP and IGNORE_NICE macros
 *	- changed downscaling cpufreq relation to the "lower way"
 *	- code/documentation cleaning
 *
 * Version 0.5 - performance and fixes
 *
 *	- completely reworked fast scaling functionality. now using a "line jump" logic instead of fixed freq "colums".
 *	  fast scaling now in 4 steps and 2 modes possible (mode 1: only fast scaling up and mode2: fast scaling up/down)
 *	- added support for "Dynamic Screen Frequency Scaling" (original implementation into zzmoove governor highly improved by Yank555)
 *	  originated by AndreiLux more info: http://forum.xda-developers.com/showpost.php?p=38499071&postcount=3
 *	- re-enabled broken conservative sampling down factor functionality ("down skip" method).
 *	  originated by Stratosk - upstream kernel 3.10rc1:
 *	  https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/log/?id=refs%2Ftags%2Fv3.10-rc1&qt=author&q=Stratos+Ka
 *	- changed down threshold check to act like it should.
 *	  originated by Stratosk - upstream kernel 3.10rc1:
 *	  https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/log/?id=refs%2Ftags%2Fv3.10-rc1&qt=author&q=Stratos+Ka
 *	- implemented/ported "early demand" from ondemand governor.
 *	  originated by Stratosk - more info: http://www.semaphore.gr/80-latests/98-ondemand-early-demand
 *	- implemented/ported "sampling down momentum" from ondemand governor.
 *	  originated by Stratosk - more info: http://www.semaphore.gr/80-latests/80-sampling-down-momentum
 *	- modified some original conservative code parts regarding frequency scaling which should work better now.
 *	  originated by DerTeufel1980: https://github.com/DerTeufel/android_kernel_samsung_smdk4412/commit/6bab622344c548be853db19adf28c3917896f0a0
 *	- added the possibility to use sampling down momentum or conservative "down skip" method.
 *	- increased possible max sampling rate sleep multiplier to 4 and sampling down factor to 100000 
 *	  accordingly to sampling down momentum implementation.
 *	- added frequency search limit for more efficient frequency searching in scaling "table" and for improving 
 *	  frequency "hard" and "soft" limit handling.
 *	- added cpu idle exit time handling like it is in lulzactive 
 *	  again work from ktoonsez : https://github.com/ktoonsez/KT747-JB/commit/a5931bee6ea9e69f386a340229745da6f2443b78
 *	  description in lulzactive governor:
 *	  https://github.com/ktoonsez/KT747-JB/blob/a5931bee6ea9e69f386a340229745da6f2443b78/drivers/cpufreq/cpufreq_lulzactive.c
 *	- fixed a little scaling step mistake and added overclocking frequencies up to 1800 mhz in scaling frequency "tables".
 *	- fixed possible freezes during start/stop/reload of governor and frequency limit change.
 *	- fixed hotplugging logic at online core 0+3 or 0+2 situations and improved hotplugging in general by
 *	  removing mutex locks and skipping hotplugging when it is not needed.
 *	- added possibility to disable hotplugging (that's a debugging relict but i thought maybe someone will find that usefull so i didn't remove it)
 *	- try to fix lags when coming from suspend if hotplug limitation at sleep was active by enabling all offline cores during resume.
 *	- code cleaning and documentation.
 *
 *	  for this functions following new tuneables were indroduced:
 *	
 *	  Early Demand:
 *	  -------------
 *	  early_demand			-> switch to enable/disable early demand functionality (possible values 0 disable or 1 enable, default: 0)
 *	  grad_up_threshold		-> scale up frequency if the load goes up in one step of grad up value (possible range from 11 to 100, default 50)
 *	                                   little example for understanding: when the load rises up in one big 50% step then the
 *	                                   frequency will be scaled up immediately instead of wating till up_threshold is reached.
 *	
 *	  Fast Scaling (improved):
 *	  ------------------------
 *	  Fast scaling has now 8 levels which at the same time have 2 modes included. Values from 1-4 equals to scaling jumps in the frequency table
 *	  and uses the Fast Scaling up but normal scaling down mode. Values from 5-8 equals to 1-4 scaling jumps but uses the fast scaling up and fast
 *	  scaling down mode.
 *
 *	  Hotplugging switch:
 *	  -------------------
 *	  disable_hotplug		-> switch to enable/disable hotplugging (possible values are any value above 0 to disable hotplugging and 0 to
 *	                                   enable it, default 0)
 *
 *	  Sampling Down Factor and Sampling Down Momentum:
 *	  ------------------------------------------------
 *	  Description: From the original author of ondemand_sampling_factor David Niemi:
 *	  "This improves performance by reducing the overhead of load evaluation and helping the CPU stay
 *	  at its top speed when truly busy, rather than shifting back and forth in speed."
 *	
 *	  And that "Sampling Down Momentum" function from stratosk does this dynamicly now! ;)
 *
 *	  sampling_down_max_momentum		-> max sampling down factor which should be set by momentum (0 disable momentum, possible range from
 *	                                           sampling_down_factor up to MAX_SAMPLING_DOWN_FACTOR, default 0 disabled)
 *	  sampling_down_momentum_sensitivity 	-> how fast the sampling down factor should be switched (possible values from 1 to 500, default 50)
 *	  sampling_down_factor			-> depending on which mode is active the factor for sampling rate multiplier which influences the whole
 *	                                           sampling rate or the value for stock "down skip" functionality which influences only the down scaling 
 *	                                           mechanism (possible values are from 1 to MAX_SMPLING_DOWN_FACTOR, default 1 disabled)
 *	
 *	  Original conservative "down skip" or "stock" method can be enabled by setting the momentum tuneable to 0. so if momentum is inactive there will 
 *	  be a fallback to the stock method. as the name "down skip" says this method works "slightly" different from the ondemand stock sampling down method 
 *	  (on which momentum was based on). It just skips the scaling down code for the given samples. if u want to completely disable the sampling down 
 *	  functionality u can achieve this by setting sampling down factor to 1. so concluded: setting sampling_down_momentum = 0 and sampling_down_factor = 1 
 *	  will disable sampling down completely (that is also the governor default setting)
 *
 *	  Dynamic Screen Frequency Scaling:
 *	  --------------------------------
 *
 *	  Dynamicly switches the screen frequency to 40hz or 60hz depending on cpu scaling and hotplug settings.
 *	  For compiling and enabling this functionality u have to do some more modification to the kernel sources, please take a look at AndreiLux Perseus
 *	  repository and there at following commit: https://github.com/AndreiLux/Perseus-S3/commit/3476799587d93189a091ba1db26a36603ee43519
 *	  After adding this patch u can enable the feature by setting "CPU_FREQ_LCD_FREQ_DFS=y" in your kernel config and if u want to check if it is
 *	  really working at runtime u can also enable the accounting which AndreiLux added by setting LCD_FREQ_SWITCH_ACCOUNTING=y in the kernel config.
 *	  If all goes well and u have the DFS up and running u can use following tuneables to do some screen magic:
 *	  (thx to Yank555 for highly extend and improving this!)
 *	
 *	  lcdfreq_enable		-> to enable/disable LCDFreq scaling (possible values 0 disable or 1 enable, default: 0)
 *	  lcdfreq_kick_in_down_delay	-> the amount of samples to wait below the threshold frequency before entering low display frequency mode (40hz)
 *	  lcdfreq_kick_in_up_delay	-> the amount of samples to wait over the threshold frequency before entering high display frequency mode (60hz)
 *	  lcdfreq_kick_in_freq		-> the frequency threshold - below this cpu frequency the low display frequency will be active
 *	  lcdfreq_kick_in_cores		-> the number of cores which should be online before switching will be active. (also useable in combination
 *	                                   with kickin_freq)
 *
 *	  So this version is a kind of "featured by" release as i took (again *g*) some ideas and work from other projects and even some of that work
 *	  comes directly from other devs so i wanna thank and give credits:
 *
 *	  First of all to stratosk for his great work "sampling down momentum" and "early demand" and for all the code fixes which found their way into
 *	  the upstream kernel version of conservative governor! congrats and props on that stratos, happy to see such a nice and talented dev directly
 *	  contibuting to the upstream kernel, that is a real enrichment for all of us!
 *
 *	  Second to Yank555 for coming up with the idea and improving/completeing (leaves nothing to be desired now *g*) my first
 *	  rudimentary implementation of Dynamic Screen Frequency Scaling from AndreiLux (credits for the idea/work also to him at this point!).
 *	
 *	  Third to DerTeufel1980 for his first implementation of stratosk's early demand functionality into version 0.3 of zzmoove governor
 *	  (even though i had to modify the original implementation a "little bit" to get it working properly ;)) and for some code optimizations/fixes
 *	  regarding scaling.
 *	
 *	  Last but not least again to ktoonsez - I "cherry picked" again some code parts of his ktoonservative governor which should improve this governor
 *	  too.
 *
 * Version 0.5.1b - bugfixes and more optimisations (in cooperation with Yank555)
 *
 *	- highly optimised scaling logic (thx and credits to Yank555)
 *	- simplified some tuneables by using already available stuff instead of using redundant code (thx Yank555)
 *	- reduced/optimised hotplug logic and preperation for automatic detection of available cores
 *	  (maybe this fixes also the scaling/core stuck problems)
 *	- finally fixed the freezing issue on governor stop!
 *
 * Version 0.6 - flexibility (in cooperation with Yank555)
 *
 *	- removed fixed scaling lookup tables and use the system frequency table instead
 *	  changed scaling logic accordingly for this modification (thx and credits to Yank555)
 *	- reduced new hotplug logic loop to a minimum
 *	- again try to fix stuck issues by using seperate hotplug functions out of dbs_check_cpu (credits to ktoonesz)
 *	- added support for 2 and 8 core systems and added automatic detection of cores were it is needed
 *	  (for setting the different core modes you can use the macro 'MAX_CORES'. possible values are: 2,4 or 8, default are 4 cores)
 *	  reduced core threshold defaults to only one up/down default and use an array to hold all threshold values
 *	- fixed some mistakes in "frequency tuneables" (Yank555):
 *	  stop looping once the frequency has been found
 *	  return invalid error if new frequency is not found in the frequency table
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------------
 *-                                                                                                                                                       -
 *---------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/earlysuspend.h>

// cpu load trigger
#define DEF_SMOOTH_UP (75)

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

// ZZ: midnight and zzmoove default values
#define DEF_FREQUENCY_UP_THRESHOLD		(70)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG	(68)	// ZZ: default for hotplug up threshold for all cpus (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD		(52)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG	(55)	// ZZ: default for hotplug down threshold for all cpus (cpu0 stays allways on)
#define DEF_IGNORE_NICE				(0)	// ZZ: default for ignore nice load
#define DEF_FREQ_STEP				(5)	// ZZ: default for freq step at awake
#define DEF_FREQ_STEP_SLEEP			(5)	// ZZ: default for freq step at early suspend

// ZZ: LCDFreq Scaling default values
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
#define LCD_FREQ_KICK_IN_DOWN_DELAY		(20)	// ZZ: default for kick in down delay
#define LCD_FREQ_KICK_IN_UP_DELAY		(50)	// ZZ: default for kick in up delay
#define LCD_FREQ_KICK_IN_FREQ			(500000)// ZZ: default kick in frequency
#define LCD_FREQ_KICK_IN_CORES			(0)	// ZZ: number of cores which should be online before kicking in
extern int _lcdfreq_lock(int lock);			// ZZ: external lcdfreq lock function
#endif

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */

#define MIN_SAMPLING_RATE_RATIO			(2)

// ZZ: Sampling down momentum variables
static unsigned int min_sampling_rate;			// ZZ: minimal possible sampling rate
static unsigned int orig_sampling_down_factor;		// ZZ: for saving previously set sampling down factor
static unsigned int orig_sampling_down_max_mom;		// ZZ: for saving previously set smapling down max momentum

// ZZ: search limit for frequencies in scaling array, variables for scaling modes and state flags for deadlock fix/suspend detection
static unsigned int max_scaling_freq_soft = 0;		// ZZ: init value for "soft" scaling = 0 full range
static unsigned int max_scaling_freq_hard = 0;		// ZZ: init value for "hard" scaling = 0 full range
static unsigned int suspend_
