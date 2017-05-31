#ifndef __ETEAM_H__
#define __ETEAM_H__


#include <linux/types.h>

#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define SYS_start_energy __NR_start_energy
#define SYS_stop_energy __NR_stop_energy
#else
#define SYS_start_energy 323
#define SYS_stop_energy 324
#endif


/**
 * Data structure that contains the information about the consumed energy of
 * a process.
 **/
struct energy {
    unsigned long long package;
    unsigned long long core;
    unsigned long long dram;
    unsigned long long gpu;
};


/**
 * Start energy measurements using E-Team for the process with the given pid.
 *
 * @param[in] pid:      The pid of the process for which energy should be measured.
 *                      Use '0' if the current process should be measured.
 *
 * @returns:            0 on success, -1 on error (setting errno accordingly)
 **/
extern int start_energy(pid_t pid);

/**
 * Stop E-Team based energy measurements for the process with the given pid.
 *
 * @param[in] pid:      The pid of the process for which energy should not be
 *                      measured anymore. Use '0' to refer to the current
 *                      process.
 *
 * @returns:            0 on success, -1 on error (setting errno accordingly)
 **/
extern int stop_energy(pid_t pid);

/**
 * Get the consumed energy for the process with the given pid.
 *
 * @param[in] pid:      The pid of the process for which the consumed energy should
 *                      be returned. Use '0' to refer to the currently running
 *                      process.
 * @param[out] energy:  Pointer to the &struct energy data structure where the final
 *                      value should be saved in.
 *
 * @returns:            0 on success, -1 on error (setting errno accordingly)
 */
extern int consumed_energy(pid_t pid, struct energy *energy);

#ifdef __cplusplus
}
#endif

#endif /* __ETEAM_H__ */
