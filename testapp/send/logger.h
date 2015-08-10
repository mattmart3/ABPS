#ifndef LOGGER_H
#define LOGGER_H

int logger_get_logid(void);
int logger_init_json(void);
void logger_mark_sent_pkt(uint32_t pkt_id, int logid);
int log_notification(struct ted_info_s *ted_info, int logid);

#endif
