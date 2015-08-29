#ifndef STRUCTS_H
#define STRUCTS_H

/* Configuration data */
struct conf_s {
	int ip_vers;
	int n_packets;
	int msg_length;
	char *iface_name;
	int iface_name_length;
};

struct ted_info_s {
	uint32_t msg_id;
	uint8_t retry_count;
	uint8_t status;
	uint16_t frag_length;
	uint16_t frag_offset;
	uint8_t more_frag;
	uint8_t ip_vers;
};


#endif
