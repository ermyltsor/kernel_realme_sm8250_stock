/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2010-2021, The Linux Foundation. All rights reserved.
 */
#ifndef __KGSL_PWRCTRL_H
#define __KGSL_PWRCTRL_H

#include <linux/clk.h>
#include <linux/pm_qos.h>
#include <soc/qcom/cx_ipeak.h>

/*****************************************************************************
 * power flags
 ****************************************************************************/
#define KGSL_PWRFLAGS_ON   1
#define KGSL_PWRFLAGS_OFF  0

#define KGSL_PWRLEVEL_TURBO 0

#define KGSL_PWR_ON	0xFFFF

#define KGSL_GPU_CFG_PATH_OFF	0
#define KGSL_GPU_CFG_PATH_LOW	1
#define KGSL_GPU_CFG_PATH_HIGH	2

#define KGSL_MAX_CLKS 18
#define KGSL_MAX_REGULATORS 2

#define KGSL_MAX_PWRLEVELS 10

/* Only two supported levels, min & max */
#define KGSL_CONSTRAINT_PWR_MAXLEVELS 2

#define KGSL_XO_CLK_FREQ	19200000
#define KGSL_RBBMTIMER_CLK_FREQ	KGSL_XO_CLK_FREQ
#define KGSL_ISENSE_CLK_FREQ	200000000

/* Symbolic table for the constraint type */
#define KGSL_CONSTRAINT_TYPES \
	{ KGSL_CONSTRAINT_NONE, "None" }, \
	{ KGSL_CONSTRAINT_PWRLEVEL, "Pwrlevel" }
/* Symbolic table for the constraint sub type */
#define KGSL_CONSTRAINT_PWRLEVEL_SUBTYPES \
	{ KGSL_CONSTRAINT_PWR_MIN, "Min" }, \
	{ KGSL_CONSTRAINT_PWR_MAX, "Max" }

#define KGSL_PWR_ADD_LIMIT 0
#define KGSL_PWR_DEL_LIMIT 1
#define KGSL_PWR_SET_LIMIT 2

/*
 * The effective duration of qos request in usecs at queue time.
 * After timeout, qos request is cancelled automatically.
 * Kept 64ms default, inline with default GPU idle time.
 */
#define KGSL_L2PC_QUEUE_TIMEOUT	(64 * 1000)

/*
 * The effective duration of qos request in usecs at wakeup time.
 * After timeout, qos request is cancelled automatically.
 */
#define KGSL_L2PC_WAKEUP_TIMEOUT (10 * 1000)

enum kgsl_pwrctrl_timer_type {
	KGSL_PWR_IDLE_TIMER,
};

/*
 * States for thermal cycling.  _DISABLE means that no cycling has been
 * requested.  _ENABLE means that cycling has been requested, but GPU
 * DCVS is currently recommending running at a lower frequency than the
 * cycle frequency.  _ACTIVE means that the frequency is actively being
 * cycled.
 */
#define CYCLE_DISABLE	0
#define CYCLE_ENABLE	1
#define CYCLE_ACTIVE	2

struct platform_device;

#ifdef CONFIG_OPLUS_FEATURE_MIDAS
// KGSL_MAX_PWRLEVELS + SLUMBER
#define MAX_GPU_PWR_STAT  KGSL_MAX_PWRLEVELS + 1
#define PWR_STAT_SLUMBER KGSL_MAX_PWRLEVELS

/**
 * struct gpu_pwr_stats - Struct holding different gpu power info obtained from
 * pwrscale and pwrctrl
 * @total:         total us seconds are sampled in this power level
 * @busy:          total busy us seconds in this power level when sampled
 * @ram_time:      total ram read+write us seconds  in this power level when sampled
 * @ram_wait:      total ram wait us seconds  in this power level when sampled
 */
struct gpu_pwr_stats {
	u64 total;
	u64 busy;
	u64 ram_time;
	u64 ram_wait;
};

/**
 * struct gpu_info - Struct holding gpu info we want to record
 * @gpu_pwr_stats:  gpu_pwr_stats of each power level + SLUMBER
 * @gpu_total:      total us seconds are sampled
 * @timestamp:      last timestamp when sampled
 * @last_state:     last KGSL device state when sampled
 */
struct gpu_info {
	struct gpu_pwr_stats gpu_pwr_stats[MAX_GPU_PWR_STAT];
	u64 gpu_total;
	ktime_t timestamp;
	int last_state;
};
#endif

struct kgsl_clk_stats {
	unsigned int busy;
	unsigned int total;
	unsigned int busy_old;
	unsigned int total_old;
};

struct kgsl_pwr_constraint {
	unsigned int type;
	unsigned int sub_type;
	union {
		struct {
			unsigned int level;
		} pwrlevel;
	} hint;
	unsigned long expires;
	uint32_t owner_id;
};

/**
 * struct kgsl_pwrlevel - Struct holding different pwrlevel info obtained from
 * from dtsi file
 * @gpu_freq:          GPU frequency vote in Hz
 * @bus_freq:          Bus bandwidth vote index
 * @bus_min:           Min bus index @gpu_freq
 * @bus_max:           Max bus index @gpu_freq
 */
struct kgsl_pwrlevel {
	unsigned int gpu_freq;
	unsigned int bus_freq;
	unsigned int bus_min;
	unsigned int bus_max;
	unsigned int acd_level;
};

struct kgsl_regulator {
	struct regulator *reg;
	char name[8];
};

/**
 * struct gpu_cx_ipeak_client - Struct holding CX Ipeak client info.
 * @client:    Client handle used for CX Ipeak vote
 * @freq:      GPU frequency threshold for which this client need to vote.
 */
struct gpu_cx_ipeak_client {
	struct cx_ipeak_client *client;
	unsigned int freq;
};

/**
 * struct kgsl_pwrctrl - Power control settings for a KGSL device
 * @interrupt_num - The interrupt number for the device
 * @grp_clks - Array of clocks structures that we control
 * @power_flags - Control flags for power
 * @pwrlevels - List of supported power levels
 * @nb - Notifier block to receive GPU OPP change event
 * @active_pwrlevel - The currently active power level
 * @previous_pwrlevel - The power level before transition
 * @thermal_pwrlevel - maximum powerlevel constraint from thermal
 * @thermal_pwrlevel_floor - minimum powerlevel constraint from thermal
 * @default_pwrlevel - device wake up power level
 * @max_pwrlevel - maximum allowable powerlevel per the user
 * @min_pwrlevel - minimum allowable powerlevel per the user
 * @num_pwrlevels - number of available power levels
 * @throttle_mask - LM throttle mask
 * @interval_timeout - timeout in jiffies to be idle before a power event
 * @clock_times - Each GPU frequency's accumulated active time in us
 * @regulators - array of pointers to kgsl_regulator structs
 * @pcl - bus scale identifier
 * @gpu_cfg - CPU to GPU AHB path bus scale identifier
 * @irq_name - resource name for the IRQ
 * @clk_stats - structure of clock statistics
 * @l2pc_cpus_mask - mask to avoid L2PC on masked CPUs
 * @l2pc_update_queue - Boolean flag to avoid L2PC on masked CPUs at queue time
 * @l2pc_cpus_qos - qos structure to avoid L2PC on CPUs
 * @pm_qos_req_dma - the power management quality of service structure
 * @pm_qos_active_latency - allowed CPU latency in microseconds when active
 * @pm_qos_cpu_mask_latency - allowed CPU mask latency in microseconds
 * @input_disable - To disable GPU wakeup on touch input event
 * @pm_qos_wakeup_latency - allowed CPU latency in microseconds during wakeup
 * @bus_control - true if the bus calculation is independent
 * @bus_mod - modifier from the current power level for the bus vote
 * @bus_percent_ab - current percent of total possible bus usage
 * @bus_width - target specific bus width in number of bytes
 * @bus_ab_mbytes - AB vote in Mbytes for current bus usage
 * @bus_index - default bus index into the bus_ib table
 * @bus_ib - the set of unique ib requests needed for the bus calculation
 * @constraint - currently active power constraint
 * @superfast - Boolean flag to indicate that the GPU start should be run in the
 * higher priority thread
 * @thermal_cycle_ws - Work struct for scheduling thermal cycling
 * @thermal_timer - Timer for thermal cycling
 * @thermal_timeout - Cycling timeout for switching between frequencies
 * @thermal_cycle - Is thermal cycling enabled
 * @thermal_highlow - flag for switching between high and low frequency
 * @limits - list head for limits
 * @limits_lock - spin lock to protect limits list
 * @sysfs_pwr_limit - pointer to the sysfs limits node
 * @cx_ipeak_pwr_limit - pointer to the cx_ipeak limits node
 * isense_clk_indx - index of isense clock, 0 if no isense
 * isense_clk_on_level - isense clock rate is XO rate below this level.
 * gpu_cx_ipeak_client - CX Ipeak clients used by GPU
 */

struct kgsl_pwrctrl {
	int interrupt_num;
	struct clk *grp_clks[KGSL_MAX_CLKS];
	struct clk *gpu_bimc_int_clk;
	int isense_clk_indx;
	int isense_clk_on_level;
	unsigned long power_flags;
	unsigned long ctrl_flags;
	struct kgsl_pwrlevel pwrlevels[KGSL_MAX_PWRLEVELS];
	struct notifier_block nb;
	unsigned int active_pwrlevel;
	unsigned int previous_pwrlevel;
	unsigned int thermal_pwrlevel;
	unsigned int thermal_pwrlevel_floor;
	unsigned int default_pwrlevel;
	unsigned int wakeup_maxpwrlevel;
	unsigned int max_pwrlevel;
	unsigned int min_pwrlevel;
	unsigned int num_pwrlevels;
	unsigned int throttle_mask;
	unsigned long interval_timeout;
	u64 clock_times[KGSL_MAX_PWRLEVELS];
	struct kgsl_regulator regulators[KGSL_MAX_REGULATORS];
	uint32_t pcl;
	uint32_t gpu_cfg;
	const char *irq_name;
	struct kgsl_clk_stats clk_stats;
	unsigned int l2pc_cpus_mask;
	bool l2pc_update_queue;
	struct pm_qos_request l2pc_cpus_qos;
	struct pm_qos_request pm_qos_req_dma;
	unsigned int pm_qos_active_latency;
	unsigned int pm_qos_cpu_mask_latency;
	unsigned int pm_qos_wakeup_latency;
	bool input_disable;
	bool bus_control;
	int bus_mod;
	unsigned int bus_percent_ab;
	unsigned int bus_width;
	unsigned long bus_ab_mbytes;
	struct device *devbw;
	unsigned int bus_index[KGSL_MAX_PWRLEVELS];
	uint64_t *bus_ib;
	struct kgsl_pwr_constraint constraint;
	bool superfast;
	struct work_struct thermal_cycle_ws;
	struct timer_list thermal_timer;
	uint32_t thermal_timeout;
	uint32_t thermal_cycle;
	uint32_t thermal_highlow;
	struct list_head limits;
	spinlock_t limits_lock;
	struct kgsl_pwr_limit *sysfs_pwr_limit;
	struct kgsl_pwr_limit *cx_ipeak_pwr_limit;
	unsigned int gpu_bimc_int_clk_freq;
	bool gpu_bimc_interface_enabled;
	struct gpu_cx_ipeak_client gpu_ipeak_client[2];
#ifdef CONFIG_OPLUS_FEATURE_MIDAS
/* gpu_stats - collect gpu power status of each pwrlevel */
	struct gpu_info gpu_stats;
#endif
};

int kgsl_pwrctrl_init(struct kgsl_device *device);
void kgsl_pwrctrl_close(struct kgsl_device *device);
void kgsl_timer(struct timer_list *t);
void kgsl_idle_check(struct work_struct *work);
void kgsl_pre_hwaccess(struct kgsl_device *device);
void kgsl_pwrctrl_pwrlevel_change(struct kgsl_device *device,
	unsigned int level);
void kgsl_pwrctrl_buslevel_update(struct kgsl_device *device,
	bool on);
int kgsl_pwrctrl_init_sysfs(struct kgsl_device *device);
void kgsl_pwrctrl_uninit_sysfs(struct kgsl_device *device);
int kgsl_pwrctrl_change_state(struct kgsl_device *device, int state);
int kgsl_clk_set_rate(struct kgsl_device *device,
	unsigned int pwrlevel);
unsigned int kgsl_pwrctrl_adjust_pwrlevel(struct kgsl_device *device,
	unsigned int new_level);
void kgsl_pwrctrl_set_thermal_cycle(struct kgsl_device *device,
	unsigned int new_level);

static inline unsigned long kgsl_get_clkrate(struct clk *clk)
{
	return (clk != NULL) ? clk_get_rate(clk) : 0;
}

/*
 * kgsl_pwrctrl_active_freq - get currently configured frequency
 * @pwr: kgsl_pwrctrl structure for the device
 *
 * Returns the currently configured frequency for the device.
 */
static inline unsigned long
kgsl_pwrctrl_active_freq(struct kgsl_pwrctrl *pwr)
{
	return pwr->pwrlevels[pwr->active_pwrlevel].gpu_freq;
}

int __must_check kgsl_active_count_get(struct kgsl_device *device);
void kgsl_active_count_put(struct kgsl_device *device);
int kgsl_active_count_wait(struct kgsl_device *device, int count);
void kgsl_pwrctrl_busy_time(struct kgsl_device *device, u64 time, u64 busy);
void kgsl_pwrctrl_set_constraint(struct kgsl_device *device,
			struct kgsl_pwr_constraint *pwrc, uint32_t id);
void kgsl_pwrctrl_update_l2pc(struct kgsl_device *device,
			unsigned long timeout_us);
int kgsl_pwrctrl_set_default_gpu_pwrlevel(struct kgsl_device *device);
void kgsl_pwrctrl_disable_unused_opp(struct kgsl_device *device,
		struct device *dev);

#ifdef CONFIG_OPLUS_FEATURE_MIDAS
void oppo_pwrctrl_update_stats_info(struct kgsl_device *device);
#endif
#endif /* __KGSL_PWRCTRL_H */
