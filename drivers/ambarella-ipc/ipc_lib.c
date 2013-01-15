#include <linux/init.h>
#include <linux/module.h>

extern int aipc_init_lk_util(void);
extern int aipc_init_lk_sdhc(void);
extern int aipc_init_lk_pmic(void);
extern int aipc_init_lk_vfs(void);
extern int aipc_init_lk_fifo(void);
extern int aipc_init_lk_host_dsp(void);
extern int aipc_init_lk_sdresp(void);
extern int aipc_init_lk_example_util(void);
extern int aipc_init_i_ffs_util(void);
extern int aipc_init_i_fifo_util(void);

static int __init ipc_lib_init(void)
{	aipc_init_lk_util();
	aipc_init_lk_sdhc();
	aipc_init_lk_pmic();
	aipc_init_lk_vfs();
	aipc_init_lk_fifo();
	aipc_init_lk_host_dsp();
	aipc_init_lk_sdresp();
	aipc_init_lk_example_util();
	aipc_init_i_ffs_util();
	aipc_init_i_fifo_util();
	return 0;
}

module_init(ipc_lib_init);