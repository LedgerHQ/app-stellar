// Structure that defines the parameters to exchange with the BOLOS UX
// application
typedef struct bolos_ux_params_s {
    // length of parameters in the u union to be copied during the syscall
    unsigned int len;
} bolos_ux_params_t;

struct ux_state_s {
    unsigned char stack_count;  // initialized @0 by the bolos ux initialize
};

typedef struct ux_state_s ux_state_t;
