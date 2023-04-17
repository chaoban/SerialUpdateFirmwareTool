#ifndef EXITSTATUS_H
#define EXITSTATUS_H

//SIS DEBUG
#define SIS_ERR         -1
#define GR_ERROR        70

#define EXIT_OK             0            /* Test pass or compare result is same */
#define EXIT_ERR           32            /* Error occurs */
#define EXIT_FAIL          33            /* Test fail or compare result is different */
#define EXIT_BADARGU       34            /* Error input argument */
#define EXIT_NODEV         35            /* Device not found */

enum CTExitCode
{
    CT_EXIT_PASS = 0,                       // CT execute done. And result pass.
    CT_EXIT_FAIL = 1,                       // CT execute done. But result fail. Such as compare different.
    CT_EXIT_CHIP_COMMUNICATION_ERROR = 2,   // SiS-Chip communication error using SiSCore. It will need to self-retry.
    CT_EXIT_AP_FLOW_ERROR = 3,              // CT check flow/somethings error. User may set some things error.
    CT_EXIT_NO_COMPORT = 4,
};
#endif
