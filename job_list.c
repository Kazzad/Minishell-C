#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "job_list.h"

// Cellule de la chaine chainee
struct job_list {
	Job job;	// Contenu
	Job_list next;	// Cellule suivante
};

Job_list list;

// Verifie si la chaine est vide
int is_job_list_empty () {
	return list == NULL;
}

// Cree une cellule a partir des attributs d'un job
// Parametres:	id		ID du job dans la liste
//		pid		PID du processus
//		status		Statut (suspendu ou actif)
//		cmd		Commande associee
Job_list create_job_list (int id, pid_t pid, status status, char* cmd) {
	// Creation de la liste
	Job_list jl = malloc (sizeof (Job_list));
	jl->next = NULL;

	// Creation du job
	jl->job = malloc (sizeof (Job));
	jl->job->id = id;
	jl->job->pid = pid;
	jl->job->status = status;
	jl->job->cmd = malloc (sizeof (char *));
	strcpy (jl->job->cmd, cmd);

	return jl;
}

// Ajoute un job a la liste à partir de ses attributs
// Parametres:	id		ID du job dans la liste
//		pid		PID du processus
//		status		Statut (suspendu ou actif)
//		cmd		Commande associee
void add_job (int id, pid_t pid, status status, char* cmd) {
	// Creation d'une cellule contenant le job
	Job_list new_job = create_job_list (id, pid, status, cmd);

	if (list == NULL) {
		// Si la liste est vide, on a termine
		list = new_job;
	} else {
		// Sinon, on ajoute la cellule a la fin de la liste
		Job_list current = list;
		while (current->next != NULL) {
			current = current->next;
		}
		// On ajoute la nouvelle cellule
		current->next = new_job;
	}
}

// Retire le job de la liste à partir de son pid
// Parametre:	pid		PID du job a retirer
void remove_job (pid_t pid) {
	if (list != NULL && list->job->pid == pid) {
		// Si la tête correspond, on a termine
		list = list->next;
	} else {
		// Sinon, on parcourt toute la liste
		Job_list current = list;
		while (current->next != NULL && current->next->job->pid != pid) {
			current = current->next;
		}
		// Si on a pas atteint la fin de la liste en sortie de boucle, on a trouve
		if (current->next != NULL ) {
			// On supprime la cellule
			current->next = current->next->next;
		}
	}
}

// Change le statut d'un job de la liste
// Parametres:	id		ID du job a modifier
//		status		nouveau statut
void change_job_status (int id, status status) {
	// On parcourt toute la liste
	Job_list current = list;
	while (current != NULL && current->job->id != id) {
		current = current->next;
	}
	// Si on a pas atteint la fin de la liste en sortie de boucle, on a trouve
	if (current != NULL) {
		// On change le statut
		current->job->status = status;
	}
}

// Obtient le pid d'un job
// Parametre:	id		ID du job
pid_t get_pid_from_id (int id) {
	// On parcourt toute la liste
	Job_list current = list;
	
	while (current != NULL && current->job->id != id) {
		current = current->next;
	}
	// Si on a pas atteint la fin de la liste en sortie de boucle, on a trouve
	if (current != NULL) {
		// On renvoie le pid
		return current->job->pid;
	} else {
		// L'id n'est pas dans la liste
		return -1;
	}
}

// Obtient l'id d'un job
// Parametre:	pid		PID du processus
pid_t get_id_from_pid (pid_t pid) {
	// On parcourt toute la liste
	Job_list current = list;
	while (current != NULL && current->job->pid != pid) {
		current = current->next;
	}
	// Si on a pas atteint la fin de la liste en sortie de boucle, on a trouve
	if (current != NULL) {
		// On renvoie l'id
		return current->job->id;
	} else {
		// Le pid n'est pas dans la liste
		return -1;
	}
}

// Affiche tous les jobs de la liste
void display_job_list () {
	// On parcourt toute la liste
	Job_list current = list;
	while (current != NULL) {
		Job job = current->job;
		// Chaine de caractere correspondant au statut
		char* string_status = job->status == Actif ? "Actif" : "Suspendu";
		// Affiche au format "[id] pid    status    cmd"
		printf("[%d] %d\t%s\t%s\n", job->id, job->pid, string_status, job->cmd);
		current = current->next;
	}
}

