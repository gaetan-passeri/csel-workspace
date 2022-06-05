---
title: CSEL - Rapport n°2
author: Muller Glenn, Passeri Gaetan
date: Juin 2022
papersize:
- a4
fontsize:
- 12pt
geometry:
- margin=1in
fontfamily:
- charter
header-includes:
- \setlength\parindent{12pt}
---

\maketitle
\thispagestyle{empty}
\clearpage
\tableofcontents
\pagenumbering{roman}
\clearpage
\pagenumbering{arabic}
\setcounter{page}{1}
\cleardoublepage

# 1. Systèmes de fichiers

# 2. Ordonnanceur
## Processus, signaux et communication

### Introduction

L'objectif de cette première partie est de créer et d'utiliser un canal de communication entre deux processus. Pour cela, on utilise l'outil __socketpair__. Cet utilitaire retourne deux descripteurs de fichiers qui permettent aux deux processus d'échanger des données par l'intermédiaire des appels systèmes __read__ et __write__. D'autre part, il est exigé que les processus tournent sur des cœurs différents du processeur. On utilise la structure __cpu_set_t__, les macros __CPU_ZERO__ et __CPU_SET__ et la fonction __sched_setaffinity()__ pour créer des sets de cœurs et les attribuer à des processus.

### Développement

Le programme comprends un processus parent et un processus enfant créé à l'aide de l'appel système __fork__. On attribue le cœur _0_ au processus parent et le cœur _1_ au processus enfant. Au démarrage, le programme crée le _fork_ et configure les _cpu sets_ pour chaque processus. Ensuite, chaque processus exécute une boucle infinie. Le processus enfant scrute l'entrée standard à l'aide de la fonction __fgets__. Lorsqu'il reçoit une chaine de caractère, il l'écrit dans le canal de communication __socketpair__ à destination du processus parent. Le parent réceptionne la chaine de caractère. S'il s'agit de la chaine "exit", il tue le processus enfant en lui faisant parvenir un signal __SIGINT__, puis se termine à son tour. Autrement, il se contente d'afficher la chaine de caractère. En réalité, pour tester le fonctionnement du programme de bout en bout, les deux processus affichent la chaine de caractère avec leurs _PID_ respectifs et la mention "parent" ou "child".

Enfin, le signal SIGINT, généré par _Ctrl-c_ est capturé afin de montrer que l'on pourrait supprimer cette possibilité pour contraindre l'utilisateur à passer par la saisie de la chaine de caractère "exit". Dans le code fourni, la fonction _sigaction()_ appelle une fonction qui quitte l'application en écrivant préalablement un message sur la sortie standard.

Voici le détail des étapes réalisées :

* __Capture du signal SIGINT__

  Dans le main, la structure suivante permet de définir un pointeur vers la fonction qui sera appelée lorsque le signal sera capturé.

  ```c
  struct sigaction act = {.sa_handler = exit_app,};
  ```

  La fonction sigaction ci-dessous fait le lien entre le signal a capturer et la structure sigaction ci-dessus.

  ```c
  int err = sigaction (SIGINT, &act, NULL);
  	if(err == -1)
  		perror("capture SIGINT signal");
  ```

  Enfin, la fonction appelée quitte l'application après avoir écrit sur la sortie standard

  ```c
  void exit_app(){
      printf("\nexit app\n");
      exit(EXIT_SUCCESS);
  }
  ```

* __Création et utilisation du canal socketpair__

  En global, un tableau de deux descripteurs de fichiers est déclaré :

  ```c
  int fd[2];
  ```

  Dans le main, on utilise la fonction __socketpair__ qui rempli ce tableau avec chacun des descripteurs :

  ```c
  err = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
  if (err == -1)
      perror("socketpair creation");
  ```

  J'ai décidé arbitrairement que le premier descripteur de fichier _fd[0]_ correspondait au parent et le second _fd[1]_ à l'enfant. Dans la routine du processus enfant, on écrit un message à destination du parent :

  ```c
  ssize_t count = write (fd[0], buf, strlen(buf));
  if (count == -1)
      perror("\nerror");
  ```

  Dans la routine du processus parent, on lis en boucle le fichier dans lequel sont écris les messages en provenance de l'enfant :

  ```c
  len = sizeof(buf);
  nr = read (fd[1], buf, len);
  if (nr == -1)
      perror("parent reading");
  ```

* __Fork__ et configuration des _cpu sets_

  Dans le main, toujours, on crée un fork du thread principal :

  ```c
  pid_t pid = fork();
  ```

  La fonction _fork_ retourne deux fois, une fois avec le processus parent et une fois avec le processus enfant fraîchement créé. Pour chacun des deux processus :

  * on déclare un set de cœurs;
  * on réinitialise le set à l'aide de la macro __CPU_ZERO__;
  * on ajoute le cœur souhaité à l'aide de la macro __CPU_SET__;
  * on attribue le set au processus correspondant;
  * on lance la routine correspondant à l'enfant / au parent.

  ```c
      if (pid == 0) {
          // ---- attribution des coeurs aux processus ----------------------------------------------
          cpu_set_t set_child;
          CPU_ZERO(&set_child);       // rst du set de cpu enfant (désattribue tous les éventuels cpu de ce set)
          CPU_SET(1, &set_child);     // ajout du coeur 1 au set de cpu enfant
          int ret = sched_setaffinity(pid, sizeof(set_child), &set_child);    // attr. du set de cpu enfant au thread enfant
          if(ret == -1)
              perror("child cpu set creation");
          // ----------------------------------------------------------------------------------------
          child_routine();
      } else if (pid > 0){
          // ---- attribution des coeurs aux processus ----------------------------------------------
          cpu_set_t set_parent;
          CPU_ZERO(&set_parent);       // rst du set de cpu parent (désattribue tous les éventuels cpu de ce set)
          CPU_SET(0, &set_parent);     // ajout du coeur 0 au set de cpu parent
          int ret = sched_setaffinity(pid, sizeof(set_parent), &set_parent);    // attr. du set de cpu parent au thread courant
          if(ret == -1)
              perror("parent cpu set creation");
          // ----------------------------------------------------------------------------------------
          parent_routine();
      } else {
          perror("fork pid return");
      }
  ```

  Dans la routine du processus enfant, on écrit le _PID_ dans une variable globale, il servira au parent à lui envoyer un signal _kill_ en temps voulu.

### Questions

## CGroups

### Introduction

### Développement

### Questions

# 3. Performances


