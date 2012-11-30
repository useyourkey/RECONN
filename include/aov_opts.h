

#ifndef _AOV_OPTS_H_
//#define LITTLE_ENDIAN

// Uncomment this out to remove USB driver dependancy from API
#define AOV_DEPEND_USB

// Uncomment this to include Engineering functions in API's
#define AOV_DEPEND_ENG

// Uncomment this to include the SA-specific functions in API's
//#define AOV_DEPEND_SA

// Unomment this to include the SA-specific functions in API's
//#define AOV_DEPEND_CUS_PACKING

// Unomment this to use Debugging (Internal)
//#define AOV_DEBUG








/* Don't Modify things below this */

/* AOV_DEPEND_ENG depends on AOV_DEPEND_SA */
#ifdef AOV_DEPEND_ENG
#ifndef AOV_DEPEND_SA
#define AOV_DEPEND_SA
#endif /* AOV_DEPEND_SA */
#endif /* AOV_DEPEND_ENG */

/* AOV_DEPEND_SA depends on AOV_DEPEND_ENCODE */
#ifdef AOV_DEPEND_SA
#ifndef AOV_DEPEND_CUS_PACKING
#define AOV_DEPEND_ENCODE
#endif /* AOV_DEPEND_CUS_PACKING */
#endif /* AOV_DEPEND_SA */

/* AOV_DEPEND_SA depends on AOV_DEPEND_ENCODE */
#ifdef AOV_DEPEND_CUS_PACKING
#define AOV_DEPEND_ENCODE
#define AOV_DOC_ENCODE
#endif /* AOV_DEPEND_CUS_PACKING */


#endif /* _AOV_OPTS_H_ */
