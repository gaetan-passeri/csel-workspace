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

## Introduction

Ce travail pratique avait pour but de nous familiariser avec l\'interaction des périphériques à l'aide du système de fichier.

## Le travail à réaliser 

Il était demandé de modifier le programme silly_led_control.c. Ce dernier est capable d\'initialiser la Led (GPIO 10) en sortie et de la faire clignoter à une fréquence de 2Hz.

Le programme doit être modifié pour permettre aux boutons poussoirs de changer la fréquence de clignotement de la Led.

-   `k1` pour augmenter la fréquence

-   `k2` pour remettre la fréquence à sa valeur initiale

-   `k3` pour diminuer la fréquence

Pour cela, il a été demandé d\'implémenter un multiplexeur d\'entrée/sortie.

Tout d\'abord, il faut initialiser les boutons poussoirs pour pouvoir interagir avec ces derniers. Pour cela, une fonction nommée open_switchs() a été implémentée :

```c
static int open_switchs(){
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K1, strlen(SW_K1));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K1, strlen(SW_K1));
    close(f);

    // config pin
    f = open(SW_K1 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);

    f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K2, strlen(SW_K2));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K2, strlen(SW_K2));
    close(f);

    // config pin
    f = open(SW_K2 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);

    f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K3, strlen(SW_K3));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K3, strlen(SW_K3));
    close(f);

    // config pin
    f = open(SW_K3 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);
    return f;

}
```

Cette fonction configure les trois boutons en mode entrée.

```c
f = open(GPIO_UNEXPORT, O_WRONLY);
write(f, SW_Kx, strlen(SW_Kx));    
close(f);
```

Le code ci-dessus permet de réinitialiser le port x. Cela permet de remettre à 0 les précédentes initialisations effectuées sur ce même port.

Ensuite, il faut exporter le port. L\'export permet d\'indiquer au système que le port souhaité va être utilisé. Cela se fait de la manière suivante :

```c
f = open(GPIO_EXPORT,O_WRONLY);
write(f, SW_Kx, strlen(SW_Kx));
close(f);
```

Une fois le port exporté, on indique la direction du port in ou out. Voici ce qu'il faut faire :

```c
f = open(SW_Kx "/direction", O_WRONLY);
write(f, "in", 2);
close(f);
return f;
```

Maintenant que nos ports sont configurés correctement, il faut configurer le multiplexeur d\'entrée/sortie. Cela se fait de la manière suivante :

```c
int epfd = epoll_create1(0);

if (epfd == -1){
	printf("ERROR : epoll create %d\n",epfd);
  exit(1);
}
```

La fonction epoll_create permet de créer un nouveau contexte epoll.

Ensuite, la création d\'un évènement doit être faite pour spécifier au contexte sur quel évènement le multiplexage doit être fait. Le code suivant montre comment faire :

```c
struct epoll_event event = {
	.events = EPOLLIN | EPOLLET,// EPOLLET,
	.data.fd = f_k1,
};
```

Maintenant, il faut ajouter le descripteur de fichier au contexte epoll. Le code suivant s\'en charge :

```c
int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k1, &event);
if (ret == -1){
  printf("ERROR : epoll ctl %d\n", ret);
	exit(1);
}    
```

Une fois le descripteur (évènement) ajouté au contexte epoll, l\'attente d\'évènement peut être faite. Pour cela la fonction epoll_wait() est utilisée de la manière suivante :

```c
int nr = epoll_wait(epfd, &event, 2, -1);
if (nr == -1)
printf("ERROR : epoll wait\n");

for (int i=0; i<nr; i++) {
	printf ("event=%ld on fd=%d\n", event.events, event.data.fd);
}
```

Event.events nous indique quel type d\'évènement a arrêté l\'attente et event.data indique quant à lui le descripteur de fichier lié à l\'évènement.

Malheureusement, la fin du travail demandé n'a pas pu être réalisée. Cela est due à une mauvaise configuration des ports en entrée. En effet, il aurait fallu rajouter un mode événementiel aux boutons. Cela se fait de la manière suivante :

```bash
echo rising > /sys/class/gpio/gpio<pin_nr>/edge
echo falling > /sys/class/gpio/gpio<pin_nr>/edge
echo both > /sys/class/gpio/gpio<pin_nr>/edge
```

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

## Introduction 

Le but des trois exercices de ce travail pratique est de faire une prise en main de l\'outil perf. En effet, c\'est un outil très complet mais également très complexe pour voir toutes ses sous-commandes exécuter la commande suivante :

    perf 

## Installation de perf

Lors de la mise en place de notre environnement Linux (TP 01), la version générée de perf ne correspond pas totalement à nos besoin.

Pour cela, il faut reconfigurer le Buildroot. Pour ce faire, exécutez les commandes suivantes :

    cd /buildroot
    make menuconfig

Une fois le menu de configuration de Buildroot chargé, allez dans **Target packages -\> Development tools** Selectionner binutils et **binutils binaries**, puis compilez le nouveau buildroot avec :

    cd /buildroot
    make

Maintenant, il faut mettre à jour le rootfs. Pour cela, exécutez le script ci-dessous :

    #!/usr/bin/env bash
    mkdir /rootfs_new/
    tar xf /buildroot/output/images/rootfs.tar -C /rootfs_new/
    rsync -acO --progress --exclude=/etc/fstab /rootfs_new/ /rootfs
    rm -Rf /rootfs_new/

Puis, générez la nouvelle version de perf avec les commandes suivantes :

    cd /buildroot/output/build/linux-5.15.21/tools/perf/
    make clean
    make ARCH=arm64 CROSS_COMPILE=/buildroot/output/host/bin/aarch64-linux-

Une fois la nouvelle version de perf générée, il faut remplacer l\'ancienne version. Pour cela, exécutez la commande suivante :

    cp perf /rootfs/usr/bin/perf

### Validation de l\'installation 

Pour s\'assurer que perf a correctement mis à jour, exécutez la commande suivante :

    perf
    branch-instructions OR branches                    [Hardware event]
    branch-misses                                      [Hardware event]
    bus-cycles                                         [Hardware event]
    cache-misses                                       [Hardware event]
    cache-references                                   [Hardware event]
    cpu-cycles OR cycles                               [Hardware event]
    instructions                                       [Hardware event]

Si une liste similaire à celle montrée ci-dessus est obtenue, alors cela signifie que la mise à jour a fonctionné.

## Compilation d\'un exemple et utilisation de perf

La commande suivante :

    perf stat ./executable 

permet d\'afficher plusieurs informations, comme le temps nécessaire à l\'exécution du programme ainsi que différentes valeurs de compteur.

Pour cette exercice, il faut compiler puis exécuter le programme ex01 de la manière suivante :

    perf stat ./ex01

Le résultat obtenu après cette exécution est le suivant :

    234      context-switches          #    5.852 /sec  
    1673821036      instructions              #    0.05  insn per cycle 
    40.039115853 seconds time elapsed
    
    39.264146000 seconds user
    0.288091000 seconds sys

Nous pouvons voir que le temps nécessaire à l\'exécution du programme est d\'environ 39,55 secondes.

### Correction

Les cache-misses sont dus à la boucle for K. C\'est pourquoi cette boucle a été enlevée et remplacée. Le code corrigé est le suivant :

    for (i = 0; i < SIZE; i++)
    {
    		for (j = 0; j < SIZE; j++)
        {
        		array[j][i] +=10;
    	 	}
    }

En effet, à chaque nouvelle itération de la boucle K, le programme devait charger l\'ensemble du tableau.

### Validation

La validation se fait à l\'aide de la commande suivante :

    perf stat -e L1-dcache-load-misses ./ex1

les résultats obtenus sont les suivants :

Performance avant correction :

    406615150      L1-dcache-load-misses       
    
    36.730038309 seconds time elapsed
    
    35.992869000 seconds user
    0.315641000 seconds sys

Performance après correction :

        42128955      L1-dcache-load-misses                                       
    
        4.497843877 seconds time elapsed
    
        4.062188000 seconds user
        0.362602000 seconds sys

On peut voir que le nombre de cache-misses et le temps d\'exécution ont été divisés par 10.

### Définition 

Instruction: Le nombre d\'instruction processeur que le programme a effectué

Cache-misses: Le nombre de donnée qui n\'était pas stocké dans le cache\
\
branch-misses: Le saut prédit par le programme (ex: une condition moins exécutée)\
\
L1-dcache-load-misses:\
\
cpu-migrations:

context-switches: Changement de contexte

### Mesure de l'impact sur la performance

Le résultats du temps d\'exécution avec time est de 35.6 secondes.

Le temps d\'exécution donné par perf stat + time est de 35.9 secondes.

On peut voir que les fonctions time n\'affectent que très peu le temps d\'exécution.

## Analyse et optimisation d'un programme

### Explication code ex02

Première étape : un tableau de short contenant 256 indice est déclaré. Ensuite, le tableau est initialisé avec des valeurs aléatoires comprises entre 0 et 251.

Deuxième étape : calcul une somme. Pour cela, deux boucles sont implémentées. La première de 1000 itérations et la deuxième de 256. La somme n\'est calculé qu\'avec des nombres plus grands que 256.

Finalement, cette somme est affichée à l\'utilisateur.

### Mesure du temps d\'exécution 

Le temps d\'exécution mesuré avec perf stat est de 26.2 secondes.

### Optimisation 

Le temps d\'exécution obtenu en ayant apporté les optimisation est de 23.4 secondes. (réalisé avec perf stat)

### Mesure 

Le programme s\'exécute plus rapidement, car le tableau est trié. En effet, la condition à l\'intérieur des boucles for ralenti l\'exécution du programme. Cela est dû au fait que le programme ne peut pas prédire la prochaine étape.

Lorsque le tableau est trié, le programme peut prédire la suite, car toutes les valeurs inférieures à 256 sont traitées en premières.

## Parsing de logs apache

#### Recherche de lenteur

L\'analyse du fichier per.data a montré que la fonction utilisant le plus la fonction std::operator== est ApacheAccessLogAnalyzer::processFile.

    26.75%  read-apache-log  read-apache-logs       [.] std::operator==<char>                                                 
    std::operator==<char>                                                                                           
    
    __gnu_cxx::__ops::_Iter_equals_val<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const>▒
    std::__find_if<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>▒
    std::__find_if<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>▒
    std::find<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, 
    HostCounter::isNewHost                                                                                                   
    HostCounter::notifyHost                                                                                                  
    ApacheAccessLogAnalyzer::processFile   

Malheureusement, la suite n'a pas pu ête réaliser par manque de temps. 
