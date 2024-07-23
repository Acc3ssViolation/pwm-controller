#ifndef LOG_H_
#define LOG_H_

void log_initialize(void);

void log_writeln(const char* string);

void log_writeln_format(const char* string, ...);

void log_write_format(const char* string, ...);

void log_write_char(const char chr);


#endif /* LOG_H_ */