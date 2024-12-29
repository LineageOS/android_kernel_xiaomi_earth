// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) "[pkuphub] " fmt

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#include <linux/platform_device.h>
#include <linux/atomic.h>

#include <hwmsensor.h>
#include <sensors_io.h>
#include "pickup_gesture.h"
#include <situation.h>
#include <hwmsen_helper.h>


#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "include/scp.h"

enum PKUPHUB_TRC {
	PKUPHUB_TRC_INFO = 0X10,
};

static struct situation_init_info pkuphub_init_info;

static int pickup_gesture_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_PICK_UP_GESTURE, &data);
	if (err < 0) {
		pr_err("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp = data.time_stamp;
	*probability = data.gesture_data_t.probability;
	return 0;
}

static int pickup_gesture_open_report_data(int open)
{
	int ret = 0;

#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_PICK_UP_GESTURE, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	ret = sensor_enable_to_hub(ID_PICK_UP_GESTURE, open);
	return ret;
}
static int pickup_gesture_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_PICK_UP_GESTURE,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

/*C3T code for HQ-268600 by chenjian at 2022/9/22 start*/
static int pickup_gesture_flush(void)
{
	return sensor_flush_to_hub(ID_PICK_UP_GESTURE);
}
/*C3T code for HQ-268600 by chenjian at 2022/9/22 end*/

/*C3T code for HQ-219034 by baoguangxiu at 2022/8/24 start*/
static int pickup_gesture_recv_data(struct data_unit_t *event,
	void *reserved)
{
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
/*C3T code for HQ-268600 by chenjian at 2022/9/22 start*/
        err = situation_flush_report(ID_PICK_UP_GESTURE);
/*C3T code for HQ-268600 by chenjian at 2022/9/22 end*/
	else if (event->flush_action == DATA_ACTION)
		err = situation_data_report_t(ID_PICK_UP_GESTURE,
				(uint32_t)event->data[0], (int64_t)event->time_stamp);
	pr_notice("pickup_gesture = %d\n",event->data[0]);
	return err;
}
/*C3T code for HQ-219034 by baoguangxiu at 2022/8/24 end*/

static int pkuphub_local_init(void)
{
	struct situation_control_path ctl = { 0 };
	struct situation_data_path data = { 0 };
	int err = 0;

	ctl.open_report_data = pickup_gesture_open_report_data;
	ctl.batch = pickup_gesture_batch;
/*C3T code for HQ-268600 by chenjian at 2022/9/22 start*/
	ctl.flush = pickup_gesture_flush;
/*C3T code for HQ-268600 by chenjian at 2022/9/22 end*/
	ctl.is_support_wake_lock = true;
	err = situation_register_control_path(&ctl, ID_PICK_UP_GESTURE);
	if (err) {
		pr_err("register pickup_gesture control path err\n");
		goto exit;
	}

	data.get_data = pickup_gesture_get_data;
	err = situation_register_data_path(&data, ID_PICK_UP_GESTURE);
	if (err) {
		pr_err("register pickup_gesture data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_PICK_UP_GESTURE,
		pickup_gesture_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit_create_attr_failed;
	}
	return 0;
exit:
exit_create_attr_failed:
	return -1;
}

static int pkuphub_local_uninit(void)
{
	return 0;
}

static struct situation_init_info pkuphub_init_info = {
	.name = "pickup_gesture_hub",
	.init = pkuphub_local_init,
	.uninit = pkuphub_local_uninit,
};

static int __init pkuphub_init(void)
{
	situation_driver_add(&pkuphub_init_info, ID_PICK_UP_GESTURE);
	return 0;
}

static void __exit pkuphub_exit(void)
{
	pr_debug("%s\n", __func__);
}

module_init(pkuphub_init);
module_exit(pkuphub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GLANCE_GESTURE_HUB driver");
MODULE_AUTHOR("hongxu.zhao@mediatek.com");
