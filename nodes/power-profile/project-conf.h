#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC nullmac_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC nullrdc_driver

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER framer_nullmac

#define ENERGEST_CONF_ON    1
#define LPM_CONF_ENABLE       1
#define LPM_CONF_MAX_PM       2
#define LPM_CONF_STATS        0

// #define CC2538_CONF_QUIET 1
// #define CC2538_RF_CONF_TX_POWER 0xFF
