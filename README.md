# Minishell-C
Projet de systèmes d'exploitation centralisés réalisé en 1ère année de formation à l'ENSEEIHT

Ce readme contient le rapport rédigé pour ce projet  


## Architecture générale:
Le programme s'organise autour de trois fichiers:  
- minishell contient le main, et gère la boucle d'exécution du shell  
- cmd_internes contient toutes les commandes internes (cd, sj, lj, ...)  
- job_list contient la liste des processus et les fonctions permettant
de la manipuler (ajouter/retirer/modifier des jobs) ou de l'interroger
(trouver un id/pid ou afficher la liste)  

## Choix et spécificités de conception:
### Gestionnaire d'erreurs
J'ai implémenté la fonction "ok" pour tester les retours des fonctions systèmes
simplement.  
### Liste des jobs
Pour optimiser l'usage de la mémoire et pour ne pas devoir limiter le nombre de
processus gérés par le minishell, j'ai fais le choix d'utiliser l'allocation
dynamique et d'implémenter une liste chaînée (LCA).  
### Gestion des signaux
Par défaut, la frappe de ctrl-C et ctrl-Z provoque l'envoi de signaux SIGINT et
SIGTSTP à tous les processus en avant plan du point de vue du shell, c'est à
dire au minishell et à ses fils.  
Pour contrôler cette diffusion, j'ai fais plusieurs choix de conception:
- les signaux SIGINT et SIGTSTP sont masqués pour les processus fils du
minishell.  
- le minishell intercepte ces signaux. A leur réception, il envoie
SIGTERM et SIGSTOP aux processus fils non en arrière plan. Ces signaux
ont un effet équivalent à SIGINT et SIGTSTP.  
Cette gestion est efficace, mais présente néanmois quelques désavantages.  
- le fils peut démasquer SIGINT et SIGTSTP. Cela peut cependant être vu
comme une liberté laissée au fils.  
- le signal SIGSTOP utilisé pour remplacer SIGTSTP ne peut pas être
bloqué, et interdit donc au fils de changer le traitant associé (ce
n'est pas le cas de SIGTERM).  
Le signal SIGCHLD est ensuite intercepté pour gérer les changements d'états des
processus et modifier la liste des jobs en conséquent, via un test du retour du
fils après waitpid.  
### Implémentation du pipeline
Le minishell créé un processus fils pour chaque commande de la ligne de commande
et la relie au processus de la précédente par un tube, de façon à ce que la
sortie de la commande précédente soit redirigée dans l'entrée de la commande
actuelle.  
L'implémentation réalisée permet de gérer un nombre quelconque de tubes.  
Ce schéma illustre les processus et tubes mis en place pour la saisie de la commande "cat f1 f2 | grep text | wc -l" :  
![](https://github.com/Kazzad/Minishell-C/blob/main/pipelines.svg?raw=true)  



