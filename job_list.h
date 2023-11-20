#ifndef JOB_LIST_C
#define JOB_LIST_C

enum status { Actif, Suspendu };
typedef enum status *status;

struct job {
	int id;		// ID du job dans la liste
	pid_t pid;	// PID du processus
	char* cmd;	// Commande executee
	status status;	// Status (suspendu ou actif)
};
typedef struct job *Job;

typedef struct job_list *Job_list;

int is_job_list_empty ();

void add_job (int id, pid_t pid, status status, char* cmd);

void change_job_status (int id, status status);

pid_t get_pid_from_id (int id);

pid_t get_id_from_pid (pid_t pid);

void remove_job (pid_t pid);

void display_job_list ();

#endif
