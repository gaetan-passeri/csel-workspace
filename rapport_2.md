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

L'objectif de cette première partie est de créer et d'utiliser un canal de communication entre deux processus. Pour cela, on utilise l'outil __socketpair__. Cet utilitaire retourne deux descripteurs de fichiers qui permettent aux deux processus d'échanger des données par l'intermédiaire des appels systèmes __read__ et __write__. D'autre part, il est exigé que les processus tournent sur des cœurs du processeur différents. On utilise la structure __cpu_set_t__, les macros __CPU_ZERO__ et __CPU_SET__ et la fonction __sched_setaffinity()__ pour créer des sets de cœurs et les attribuer aux processus.

### Développement

Le programme comprends un processus parent - qui est le processus principal - et un processus enfant créé à l'aide de l'appel système __fork__. On attribue le cœur _0_ au processus parent et le cœur _1_ au processus enfant. Au démarrage, le programme crée le _fork_ et configure les _cpu sets_ pour chaque processus. Ensuite, chaque processus exécute une boucle infinie. Le processus enfant scrute l'entrée standard à l'aide de la fonction __fgets__. Lorsqu'il reçoit une chaine de caractère, il l'écrit dans le canal de communication __socketpair__ à destination du processus parent. Le parent réceptionne la chaine de caractère. S'il s'agit de la chaine "exit", il tue le processus enfant en lui faisant parvenir un signal __SIGKILL__, puis se termine à son tour. Autrement, il se contente d'afficher la chaine de caractère. En réalité, pour tester le fonctionnement du programme de bout en bout, les deux processus affichent la chaine de caractère avec leurs _PID_ respectifs et la mention "parent" ou "child".

Le signal SIGINT, généré par un éventuel _Ctrl-c_ est capturé afin de montrer que l'on pourrait supprimer cette possibilité pour contraindre l'utilisateur à passer par la saisie de la chaine de caractère "exit". Dans le code fourni, la fonction _sigaction()_ appelle une fonction qui quitte l'application en écrivant préalablement un message sur la sortie standard.

Voici le détail des étapes réalisées :

#### Capture du signal SIGINT

Dans le main, la structure suivante permet de définir un pointeur vers la fonction appelée lorsque le signal est capturé.

```c
struct sigaction act = {.sa_handler = exit_app,};
```

La fonction sigaction ci-dessous fait le lien entre le signal à capturer et la structure sigaction ci-dessus.

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

#### Création et utilisation du canal socketpair

On déclare un tableau de deux descripteurs de fichiers en variable globale :

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

#### Fork et configuration des _cpu sets_

Dans le main, on crée un fork du thread principal :

```c
pid_t pid = fork();
```

La fonction _fork_ retourne deux fois. Une fois avec le _PID_ du processus parent et une fois avec celui du processus enfant fraîchement créé. Pour chacun des deux processus :

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

#### Fermeture de l'application

Au début de l'execution de la routine enfant, avant d'entrer dans la boucle infinie, le processus écrit son _PID_ dans une variable globale. Il servira au parent lorsque ce dernier aura besoin de lui envoyer un signal __SIGKILL__.

Lorsque le message reçu par le processus parent correspond au mot "exit", il envoi un signal __SIGKILL__ au processus enfant à l'aide de l'appel système __kill__. Puis, il se termine lui-même.

```c
if(strcmp(buf, "exit") == 0){            
    // kill child process
    int ret = kill(child_pid, SIGKILL);
    if (ret == -1) {
        perror("kill");
        exit(EXIT_FAILURE);
    }

    // exit main process
    exit(EXIT_SUCCESS);
}
```

### Résultat

En execurant le programme, lors de la saisie d'une chaine de caractère, on constate que le processus enfant communique bien le message au processus parent.

````bash
hello
received msg in child (pid 259) : hello
received msg in parent (pid 258) : hello
````

Lorsqu'on passe le processus en tâche de fond en actionnant les touches _Ctrl-z_ suivies de la commande _bg_, on peut utiliser la commande _ps_ pour afficher la liste des processus. On vérifie ainsi que les deux _pid_ affichés par le programme correspondent bien.

```bash
# ps
  PID TTY          TIME CMD
  252 pts/0    00:00:00 sh
  258 pts/0    00:00:00 app
  259 pts/0    00:00:00 app
  271 pts/0    00:00:00 ps
#
```

## CGroups

### Introduction

Les groupes de contrôles _CGroups_ Linux permettent de limiter les ressources mémoire et CPU attribuées à des processus. Il est également possible de contrôler l'utilisation d'autres périphériques, comme par exemple les entrées / sorties (GPIO). Le but final est de créer des sous-systèmes disposant de certaines quantités de ressources maximale. Ceci permet de séparer les ressources du processeur par activités.

### Contrôle des ressources mémoire

* Dans un terminal du système embarqué, on commence par monter _cgroup_ dans le rootfs :

    ```bash
    $ mount -t tmpfs none /sys/fs/cgroup
    ```

* On crée ensuite un groupe de contrôle en y ajoutant un répertoire portant son nom. Dans notre cas, on souhaite contrôler l'utilisation de la mémoire uniquement. On appelle donc ce groupe "memory".

    ```bash
    $ mkdir /sys/fs/cgroup/memory
    ```

* On monte le groupe en question en spécifiant les sous-systèmes que l'on souhaite y afficher à l'aide de l'option __-o__. Dans le cas ci-dessous, seuls les sous-systèmes permettant de contrôler la mémoire seront visibles.

    ```bash
    $ mount -t cgroup -o memory memory /sys/fs/cgroup/memory
    ```

* à la racine du réperoite `/sys/fs/cgroup/memory` se trouve tous les sous-systèmes pour la configuration générale de la mémoire. Etant donné que l'on souhaite créer un groupe séparé dans lequel limiter la mémoire de certains processus, on crée un répertoire "mem" dans `sys/fs/cgroup/memory`. Ce répertoire contiendra tous les mêmes éléments que son répertoire parent et pourra être configuré différement.

    ```bash
    $ mkdir /sys/fs/cgroup/memory/mem
    ```

* Enfin, il suffit de configurer ce groupe de contrôle pour limiter la mémoire d'un ou plusieurs processus. On limite le nombre d'octets disponibles en l'écrivant dans la propriété _memory.limit_in_bytes_.

    ```bash
    $ echo 20M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes
    ```

    Voici la valeur affichée lors de la re-lecture de cet attribut:

    ```bash
    # cat /sys/fs/cgroup/memory/mem/memory.limit_in_bytes
    20971520
    ```

* On peut maintenant ajouter les processus auxquels on souhaite imposer cette limitation au groupe de contrôle. Dans l'exemple ci-dessous, "$$" correspondant au processus courant, on ajoute le processus au _CGroup_.

    ```bash
    $ echo $$ > /sys/fs/cgroup/memory/mem/tasks
    ```

    Voici la liste des processus qui respectent cette limitation :

    ```bash
    # cat /sys/fs/cgroup/memory/mem/tasks
    245
    283
    ```

### Vérification de la limitation de mémoire accessible

En principe, la mémoire RAM totale à disposition de ces processus ainsi que de leurs éventuels enfants devrait être limitée à 20 MB. Pour m'en assurer, je vais créer un programme qui va tenter d'allouer une mémoire plus grande et de la remplir avec des zéros :

```c
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>      // gestion des messages d'erreurs

#define SIZE_IN_BYTES 20000000

int main(int argc, char* argv[])
{
    // allocate 20 MB of memory
    char *ptr;
    ptr = malloc(SIZE_IN_BYTES);
    if(ptr == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }

    // fill memory with zeroes
    for(int i=0; i<SIZE_IN_BYTES; i++){
        ptr[i] = 0;
    }

    // print memory content
    for(int i=0; i<SIZE_IN_BYTES; i++){
        printf("%d",ptr[i]);
    }

    // free memory
    free(ptr);
    ptr = NULL;
    
    return 0;
}
```

Dans le programme ci-dessus, lorsque la définition __SIZE_IN_BYTES__ est à 20M, limite de la mémoire utilisée dans notre cas, le programme se comporte comme prévu. En revanche, si on passe la quantité d'octets à allouer à 30M, le processus est "tué" immédiatement, comme le montre l'extrait du terminal suivant :

```bash
# ./app
Killed
```

On peut vérifier la mémoire encore disponible à tout moment en lisant la propriété __memory.usage_in_bytes__ de la façon suivante :

```bash
cat /sys/fs/cgroup/memory/mem/memory.usage_in_bytes
```

### Contrôle des ressources CPU et vérification

On souhaite à présent limiter l'utilisation des cœurs du processeur à un certain nombre pour les processus concernés. Pour ce faire, on commence par ajouter un groupe de contrôle __cpuset__ dans le _CGroup_ de l'exercice précédent. On crée les deux sous-groupes _high_ et _low_ pour le contrôle des CPU.

Les quatre dernières lignes de l'extrait de code ci-dessous configurent les cœurs qui peuvent être utilisés par les sous-groupes _low_ et _high_. Par exemple, dans notre cas, tous les processus placés dans le sous-groupe _low_ seront exécutés dans le cœur 2 du processeur alors que tous les processus placés dans le sous-groupe _high_ seront exécutés dans le cœur 3.

```bash
$ mkdir /sys/fs/cgroup/cpuset
$ mount -t cgroup -o cpu,cpuset cpuset /sys/fs/cgroup/cpuset
$ mkdir /sys/fs/cgroup/cpuset/high
$ mkdir /sys/fs/cgroup/cpuset/low
$ echo 3 > /sys/fs/cgroup/cpuset/high/cpuset.cpus
$ echo 0 > /sys/fs/cgroup/cpuset/high/cpuset.mems
$ echo 2 > /sys/fs/cgroup/cpuset/low/cpuset.cpus
$ echo 0 > /sys/fs/cgroup/cpuset/low/cpuset.mems
```

Le code de test se contente de faire un _fork_ et d'exécuter dans chaque processus une incrémentation d'une variable dans une boucle infinie. En lançant le programme sur la cible en tâche de fond et en utilisant la commande _htop_, on observe que deux des CPUs sont bien utilisés à 100%. La liste des processus permet de confirmer que les coupables sont les deux processus de notre application (nom de l'executable : "app")!

```bash
top - 03:49:30 up  2:14,  1 user,  load average: 2.07, 1.81, 1.04
Tasks:  95 total,   3 running,  92 sleeping,   0 stopped,   0 zombie
%Cpu0  :   0.0/0.0     0[                                                                                                    ]
%Cpu1  : 100.0/0.0   100[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
%Cpu2  : 100.0/0.0   100[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
%Cpu3  :   0.6/1.3     2[||                                                                                                  ]
GiB Mem :  8.7/0.5      [                                                                                                    ]
GiB Swap:  0.0/0.0      [                                                                                                    ]

  PID USER      PR  NI    VIRT    RES  %CPU  %MEM     TIME+ S COMMAND
    1 root      20   0    2.9m   0.3m   0.0   0.1   0:01.78 S init
  160 root      20   0    2.9m   0.3m   0.0   0.1   0:00.01 S  `- /sbin/syslogd -n
  164 root      20   0    2.9m   0.3m   0.0   0.1   0:00.01 S  `- /sbin/klogd -n
  176 root      20   0    3.3m   2.2m   0.0   0.5   0:00.14 S  `- /sbin/mdev -df
  240 root      20   0    6.3m   2.0m   0.0   0.4   0:00.00 S  `- sshd: /usr/sbin/sshd [listener] 0 of 10-100 startups
  243 root      20   0    6.4m   4.5m   0.0   0.9   0:04.36 S      `- sshd: root@pts/0
  245 root      20   0    2.9m   2.2m   0.0   0.5   0:00.23 S          `- -sh
  312 root      20   0    2.1m   0.2m 100.0   0.0  10:45.06 R              `- ./app
  313 root      20   0    2.1m   0.1m  99.4   0.0  10:44.96 R                  `- ./app
  315 root      20   0    3.4m   2.2m   1.3   0.5   0:08.71 R              `- top                                                                                                                                
```

Si on ouvre deux terminaux et que l'on ajoute le processus courant de chacun dans les sous groupes, respectivement _low_ et _high_, on constate que lors de l'exécution du programme sur un de ces terminaux, les deux processus du programme ne s'exécutent que dans le cœur configuré pour le sous-groupe correspondant.

En revanche, si on mets à disposition tous les cœurs pour un sous-groupe, par exemple, pour le sous-groupe _high_ : 

```bash
echo 0 1 2 3 > /sys/fs/cgroup/cpuset/high/cpuset.cpus
```

et que l'on exécute le programme dans le terminal qui y a été placé, les deux processus du programme utilisent les quatre cœurs du processeurs. Tous sont alors saturés.

L'attribut __cpu.shares__ permet de répartir le temps CPU entre différents groupes de contrôle. Si on souhaitait exécuter deux processus sur un même cœur et attribuer 25% de temps CPU à un processus et 75% à l'autre, il faudrait créer deux groupes, attribuer le même cœur à ces deux groupes, et configurer différemment l'attribut __cpu.shares__ pour chacun.

# 3. Performances

