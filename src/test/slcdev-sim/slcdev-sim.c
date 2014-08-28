#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/dcclog.h>
#include "slcdev.h"

void dev_sim_enable(bool mod, unsigned int addr)
{
	if (addr > 160) 
		return;

	ss_dev_tab[addr + (mod ? 160 : 0)].enabled = 1;
}

void dev_sim_disable(bool mod, unsigned int addr)
{
	if (addr > 160) 
		return;

	ss_dev_tab[addr + (mod ? 160 : 0)].enabled = 0;
}

void dev_sim_multiple_disable(uint32_t s[], uint32_t m[])
{
	int n;

	for (n = 0; n < 160; ++n) {
		int j = n / 32; 
		int i = n % 32; 
		/* disable sensors */
		if ((s != NULL) && (s[j] & (1 << i))) {
			ss_dev_tab[n].enabled = 0;
		}
		/* disable modules */
		if ((m != NULL) && (m[j] & (1 << i)))
			ss_dev_tab[160 + n].enabled = 0;
	}
}

void dev_sim_multiple_enable(uint32_t s[], uint32_t m[])
{
	int n;

	for (n = 0; n < 160; ++n) {
		int j = n / 32; 
		int i = n % 32; 
		/* enable sensors */
		if ((s != NULL) && (s[j] & (1 << i)))
			ss_dev_tab[n].enabled = 1;
		/* enable modules */
		if ((m != NULL) && (m[j] & (1 << i)))
			ss_dev_tab[160 + n].enabled = 1;
	}
}

void dev_sim_multiple_alarm_set(uint32_t s[], uint32_t m[], unsigned int lvl)
{
	int n;

	for (n = 0; n < 160; ++n) {
		int j = n / 32; 
		int i = n % 32; 
		/* enable sensors */
		if ((s != NULL) && (s[j] & (1 << i)))
			ss_dev_tab[n].alm = lvl;
		/* enable modules */
		if ((m != NULL) && (m[j] & (1 << i)))
			ss_dev_tab[160 + n].alm = lvl;
	}
}

void dev_sim_multiple_trouble_set(uint32_t s[], uint32_t m[], unsigned int lvl)
{
	int n;

	for (n = 0; n < 160; ++n) {
		int j = n / 32; 
		int i = n % 32; 
		/* enable sensors */
		if ((s != NULL) && (s[j] & (1 << i)))
			ss_dev_tab[n].tbl = lvl;
		/* enable modules */
		if ((m != NULL) && (m[j] & (1 << i)))
			ss_dev_tab[160 + n].tbl = lvl;
	}
}

struct ss_device * dev_sim_lookup(bool mod, unsigned int addr) 
{
	if (addr > 160) 
		return NULL;

	return &ss_dev_tab[addr + (mod ? 160 : 0)];
}

struct microjs_script {
	int x;
};

void sim_js_exec(struct ss_device * dev, struct db_dev_model * model, 
				 struct microjs_script * cmd)
{

}

#define REMOTE_TEST_MSK 0x2d /* 101101 */
#define REMOTE_TEST_ON  0x00 
#define REMOTE_TEST_OFF 0x2d 

/* Default control bits processing for sensors */
void sensor_ctl_default(struct ss_device * dev, 
					   struct db_dev_model * model, uint32_t ctl)
{
	/* Remote test */
	switch (ctl & REMOTE_TEST_MSK) {
	case REMOTE_TEST_ON:
		DCC_LOG(LOG_TRACE, "Remote test enabled");
		dev->tst = 1;
		dev->pw2 = device_db_pw_lookup(model->pw2, 1);
		dev->pw4 = device_db_pw_lookup(model->pw4, 3);
		break;
	case REMOTE_TEST_OFF:
		DCC_LOG(LOG_TRACE, "Remote test disabled");
		dev->tst = 0;
		dev->pw2 = device_db_pw_lookup(model->pw2, 0);
		dev->pw4 = device_db_pw_lookup(model->pw4, 0);
		break;
	}
}

/* simulate a custom sensor */
void sensor_sim_custom(struct ss_device * dev, 
					   struct db_dev_model * model, uint32_t ctl)
{
	struct cmd_list * lst;

	sensor_ctl_default(dev, model, ctl);

	/* execute commands from the device model */
	if ((lst = model->cmd) != NULL) {
		int i;
		for (i = 0; i < lst->cnt; ++i) {
			struct cmd_entry * cmd = &lst->cmd[i];
			if ((ctl & cmd->seq.msk) == cmd->seq.val) {
				DCC_LOG1(LOG_TRACE, "CMD[%d]", i);
				sim_js_exec(dev, model, cmd->script);
			}
		}
	}
}

/* simulate a photodetector smoke sensor */
void sensor_sim_photo(struct ss_device * dev, 
					  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG(LOG_INFO, "...");

	sensor_ctl_default(dev, model, ctl);
}

/* simulate a ion smoke detector */
void sensor_sim_ion(struct ss_device * dev, 
					struct db_dev_model * model, uint32_t ctl)
{
	int lvl;

	sensor_ctl_default(dev, model, ctl);

	if ((lvl = dev->alm) > 0) {
		if (lvl > 3)
			lvl = 3;
		DCC_LOG1(LOG_TRACE, "Alarm %d", dev->alm);
		dev->pw4 = device_db_pw_lookup(model->pw4, lvl + 2);
	} else if ((lvl = dev->tbl) > 0) {
		if (lvl > 2)
			lvl = 2;
		DCC_LOG1(LOG_TRACE, "Trouble %d", dev->alm);
		dev->pw4 = device_db_pw_lookup(model->pw4, lvl);
	} else {
		dev->pw4 = device_db_pw_lookup(model->pw4, 0);
	}
}

/* simulate a heat detector sensor */
void sensor_sim_heat(struct ss_device * dev, 
					 struct db_dev_model * model, uint32_t ctl)
{
	int lvl;

	sensor_ctl_default(dev, model, ctl);

	if ((lvl = dev->alm) > 0) {
		if (lvl > 2)
			lvl = 2;
		DCC_LOG1(LOG_TRACE, "Alarm %d", dev->alm);
		dev->pw4 = device_db_pw_lookup(model->pw4, lvl);
	} else {
		dev->pw4 = device_db_pw_lookup(model->pw4, 0);
	}
}

/* simulate an Acclimate Photoelectric Smoke Sensor */
void sensor_sim_acclimate(struct ss_device * dev, 
						  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG(LOG_INFO, "...");

	sensor_ctl_default(dev, model, ctl);
}

/* simulate an Beam Smoke Sensor */
void sensor_sim_beam(struct ss_device * dev, 
						  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG(LOG_INFO, "...");

	sensor_ctl_default(dev, model, ctl);
}

/* simulate a COPTIR Smoke Detector Sensor */
void sensor_sim_coptir(struct ss_device * dev, 
					   struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG(LOG_INFO, "...");

	sensor_ctl_default(dev, model, ctl);
}



/* simulate a custom module */
void module_sim_custom(struct ss_device * dev, 
					   struct db_dev_model * model, uint32_t ctl)
{
	struct cmd_list * lst;

	/* execute commands from the device model */
	if ((lst = model->cmd) != NULL) {
		int i;
		for (i = 0; i < lst->cnt; ++i) {
			struct cmd_entry * cmd = &lst->cmd[i];
			if ((ctl & cmd->seq.msk) == cmd->seq.val) {
				DCC_LOG1(LOG_TRACE, "CMD[%d]", i);
				sim_js_exec(dev, model, cmd->script);
			}
		}
	}
}

#define CONTROL_OUT_MSK 0x2d /* 101101 */
#define CONTROL_OUT_ON  0x00 
#define CONTROL_OUT_OFF 0x2d 


void module_contorl_seq(struct ss_device * dev, 
						struct db_dev_model * model, uint32_t ctl)
{
	switch (ctl & 0x7) {
	case 0:/* 0 0 0 */
		/* Outputs:         1  2  3  5 */
		dev->out1 = 1; /* Pulse */
		dev->out2 = 1; 
		dev->out3 = 1;
		dev->out5 = 1;
		DCC_LOG(LOG_MSG, "Pu 1  1  1");
		break;
	case 4: /* 0 0 1 */
		dev->out1 = 1; /* Pulse */
		dev->out3 = 1;
		dev->out5 = 1;
		DCC_LOG(LOG_MSG, "Pu NC 1  1");
		break;
	case 2: /* 0 1 0 */
		dev->out1 = 0;
		dev->out2 = 1;
		dev->out3 = 1;
		dev->out5 = 1;
		DCC_LOG(LOG_MSG, "0  1  1  1");
		break;
	case 6: /* 0 1 1 */
		dev->out1 = 0;
		dev->out3 = 1;
		dev->out5 = 1;
		DCC_LOG(LOG_MSG, "0  NC 1  1");
		break;
	case 1: /* 1 0 0 */
		dev->out1 = 1; /* Pulse */
		dev->out3 = 0;
		dev->out5 = 0;
		DCC_LOG(LOG_MSG, "Pu NC 0  0");
		break;
	case 5: /* 1 0 1 */
		dev->out1 = 0; /* Pulse */
		dev->out2 = 0;
		dev->out3 = 1; /* Pulse */
		dev->out5 = 1; /* Pulse */
		DCC_LOG(LOG_MSG, "Pu 0  Pu Pu");
		break;
	case 3: /* 1 1 0 */
		dev->out1 = 0;
		dev->out2 = 0;
		dev->out5 = 0;
		DCC_LOG(LOG_MSG, "0  NC 0  0");
		break;
	case 7: /* 1 1 1 */
		dev->out1 = 0;
		dev->out2 = 0;
		dev->out3 = 1; /* Pulse */
		dev->out5 = 1; /* Pulse */
		DCC_LOG(LOG_MSG, "0  0  Pu Pu");
		break;
	}

	DCC_LOG4(LOG_TRACE, "%d %d %d %d",
			 dev->out1, dev->out2, dev->out3, dev->out5);
}


/* simulate a relay module */
void module_sim_relay(struct ss_device * dev, 
					  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_TRACE, "addr=%d ctl=0x%04x", dev->addr, ctl);

	module_contorl_seq(dev, model, ctl);

	if ((ctl & 0x81) == 0) {
		/* 1.	Bit 10 = 0,  sent two consecutive times, 
		   will reset PW3 latches */
		DCC_LOG(LOG_TRACE, "Reset PW3 latches.");
	}

	switch (ctl & CONTROL_OUT_MSK) {
	case CONTROL_OUT_ON:
		DCC_LOG(LOG_TRACE, "Set");
		dev->pw2 = device_db_pw_lookup(model->pw2, 1);
		break;

	case CONTROL_OUT_OFF:
		DCC_LOG(LOG_TRACE, "Reset");
		dev->pw2 = device_db_pw_lookup(model->pw2, 0);
		break;
	}
}

/* simulate a supervised cntrol module */
void module_sim_control(struct ss_device * dev, 
						struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_INFO, "addr=%d ctl=0x%04x", dev->addr, ctl);

	module_contorl_seq(dev, model, ctl);

	switch (ctl & CONTROL_OUT_MSK) {
	case CONTROL_OUT_ON:
		DCC_LOG(LOG_TRACE, "Control ON");
		dev->pw2 = device_db_pw_lookup(model->pw2, 1);
		break;
	case CONTROL_OUT_OFF:
		DCC_LOG(LOG_TRACE, "Control Off");
		dev->pw2 = device_db_pw_lookup(model->pw2, 0);
		break;
	}
}

/* simulate a firefighter telephone module */
void module_sim_phone(struct ss_device * dev, 
					  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_TRACE, "addr=%d ctl=0x%04x", dev->addr, ctl);

	module_contorl_seq(dev, model, ctl);

	switch (ctl & CONTROL_OUT_MSK) {
	case CONTROL_OUT_ON:
		DCC_LOG(LOG_TRACE, "Set");
		dev->pw2 = device_db_pw_lookup(model->pw2, 1);
		break;

	case CONTROL_OUT_OFF:
		DCC_LOG(LOG_TRACE, "Reset");
		dev->pw2 = device_db_pw_lookup(model->pw2, 0);
		break;
	}
}

#define CLASS_A_MSK      0x2d /* 101101 */
#define CLASS_A_SWITCHED 0x00 
#define CLASS_A_NORMAL   0x2d 

/* simulate a monitor module */
void module_sim_monitor(struct ss_device * dev, 
						struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_INFO, "addr=%d ctl=0x%04x", dev->addr, ctl);

	switch (ctl & CLASS_A_MSK) {
	case CLASS_A_SWITCHED:
		DCC_LOG(LOG_TRACE, "Class A switched");
		dev->pw2 = device_db_pw_lookup(model->pw2, 1);
		break;
	case CLASS_A_NORMAL:
		DCC_LOG(LOG_TRACE, "Class A normal");
		dev->pw2 = device_db_pw_lookup(model->pw2, 0);
		break;
	}
}

/* simulate a mini-module */
void module_sim_mini(struct ss_device * dev, 
					 struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_TRACE, "addr=%d ctl=0x%04x", dev->addr, ctl);
}

/* simulate a 2 wire module */
void module_sim_2wire(struct ss_device * dev, 
					  struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_TRACE, "addr=%d ctl=0x%04x", dev->addr, ctl);
}

/* simulate a 4-20ma input device */
void module_sim_4_20ma(struct ss_device * dev, 
					   struct db_dev_model * model, uint32_t ctl)
{
	DCC_LOG2(LOG_TRACE, "addr=%d ctl=0x%04x", dev->addr, ctl);
}

#define SIM_MODEL_NAME_MAX 12

struct sim_model {
	const char name[SIM_MODEL_NAME_MAX]; 
	void (* run)(struct ss_device * dev, 
				 struct db_dev_model * model, 
				 uint32_t ctl);
};

const struct sim_model sim_model_lut[] = {
	[0] = { .name = "custom", .run = sensor_sim_custom },
	[1] = { .name = "photo", .run = sensor_sim_photo },
	[2] = { .name = "ion", .run = sensor_sim_ion },
	[3] = { .name = "heat", .run = sensor_sim_heat },
	[4] = { .name = "acclimate", .run = sensor_sim_acclimate },
	[5] = { .name = "beam", .run = sensor_sim_beam },
	[6] = { .name = "coptir", .run = sensor_sim_coptir },

	[7] = { .name = "custom", .run = module_sim_custom },
	[8] = { .name = "relay", .run = module_sim_relay },
	[9] = { .name = "control", .run = module_sim_control },
	[10] = { .name = "monitor", .run = module_sim_monitor },
	[11] = { .name = "mini", .run = module_sim_mini },
	[12] = { .name = "2wire", .run = module_sim_2wire },
	[13] = { .name = "phone", .run = module_sim_phone },
	[14] = { .name = "4-20mA", .run = module_sim_4_20ma }
};

#define SENSOR_SIM_CUSTOM 0
/* XXX: changes in the table bellow must be refelected in 
   this two constants */
#define MODULE_SIM_CUSTOM 7
#define MODEL_SIM_LAST 13

int sensor_sim_lookup(const char * name, unsigned int len)
{
	int i;

	for (i = SENSOR_SIM_CUSTOM; i < MODULE_SIM_CUSTOM; ++i) {
		if ((strncmp(name, sim_model_lut[i].name, len) == 0) && 
			(strlen(sim_model_lut[i].name) == len))
			return i;
	}

	return SENSOR_SIM_CUSTOM;
}

int sensor_sim_default(void)
{
	return SENSOR_SIM_CUSTOM;
}

int module_sim_lookup(const char * name, unsigned int len)
{
	int i;

	for (i = MODULE_SIM_CUSTOM; i <= MODEL_SIM_LAST; ++i) {
		if ((strncmp(name, sim_model_lut[i].name, len) == 0) && 
			(strlen(sim_model_lut[i].name) == len))
			return i;
	}

	return MODULE_SIM_CUSTOM;
}

int module_sim_default(void)
{
	return MODULE_SIM_CUSTOM;
}

const char * model_sim_name(unsigned int idx)
{
	if (idx > MODEL_SIM_LAST)
		return "";

	return sim_model_lut[idx].name;
}

void __attribute__((noreturn)) sim_event_task(void)
{
	uint32_t event;
	struct ss_device * dev;
	struct db_dev_model * model;
	uint32_t ctl;

	thinkos_sleep(3000);

	slcdev_resume();

	for (;;) {
		event = slcdev_event_wait();
		dev = slcdev_drv.dev;
		ctl = slcdev_drv.ctls;
		/* get the model for this device */
		model = db_dev_model_by_index(dev->model);

		if (event & SLC_EV_TRIG) {
			DCC_LOG1(LOG_INFO, "trigger %d", dev->addr);
			led_flash(0, 64);
		}

		if ((event & SLC_EV_SIM) && (model != NULL)) {
			const struct sim_model * sim;
			sim = &sim_model_lut[model->sim];

			DCC_LOG2(LOG_INFO, "dev=%d ctl=0x%x", dev->addr, ctl);

			/* Poll LED state */
			if ((ctl & 0x4) == 0) {
				dev->led = 1;
				led_on(1);
			} else if ((ctl & 0x5) == 4) {
				dev->led = 0;
				led_off(1);
			} else if ((ctl & 0x5) == 5) {
				led_flash(1, 64);
			}

			sim->run(dev, model, ctl);
		}
	}
}

