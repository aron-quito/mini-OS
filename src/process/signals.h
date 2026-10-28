#ifndef SIGNALS_H
#define SIGNALS_H

/**
 * @brief Configura manejadores de señales (SIGINT, SIGCHLD, SIGTSTP) usando sigaction.
 */
void signals_init(void);

#endif // SIGNALS_H
