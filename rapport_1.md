# 1. Environnement Linux embarqué (du 25.02 au 04.03)

## Résumé

### Installation

* Installer docker

* Installer git => configurer git pour utiliser wsl2 au lieu de poweshell => depuis poweshell :

  ````bash
  git config --global core.autocrlf false
  ````

* Installer vscode

* fork du projet du cours https://github.com/supcik/csel-workspace et ajout des collaborateurs

* ouvrir vscode, le projet contient un environnement de developpement docker => accepter de relancer le projet dans un container docker. => le projet se relance, le terminal de vscode est un linux.

### Buildroot

* depuis le terminal de vscode, lancer la commande suivante pour télécharger buildroot :

  ````bash
  ./get-buildroot.sh 
  ````

* une fois télécharger, lancer la commande suivante pour configurer buildroot si nécessaire :

  ````bash
  cd buildroot
  make menuconfig
  ````

* et compiler buildroot avec :

  ````bash
  make
  ````

  __NB__ : la compilation dure environ une heure.

* actualiser le __root filesystem__

  ````bash
  rm -Rf /rootfs/*
  tar xf /buildroot/output/images/rootfs.tar -C /rootfs
  ````

### Gravure de la carte SD

* copier les images dans le répertoire synchronisé avec la machine de developpement

  ````bash
  rsync -rlt --progress --delete /buildroot/output/images/ /workspace/buildroot-images
  ````

* copier les images de buildroot qui se trouvent dans le répertoire _/buildroot/output/images/_ dans le répertoire partagé avec l'ordinateur _workspace_ 

  ````bash
  rsync -rlt --progress --delete /buildroot/output/images/ /workspace/buildroot-images
  ````

  __NB__ : la commande __rsync__ propose plus d'option que __cp__.

* Pour flasher la carte SD, on utilise le logiciel __Balena Etcher__ https://www.balena.io/etcher/

  * Insérer la carte SD dans la machine de développement
  * sélectionner l’image `buildroot-images/sdcard.img`
  * sélectionner le disque qui correspond à la carte SD et cliquer sur “Flash!”. (carte SD de 16GB en l'occurence)

### Communication avec la cible

* Installation du logiciel __Putty__ sur Windows (il existe de nombreux autres programmes pour Linux) [`putty-64bit-0.76-installer.msi`](https://the.earth.li/~sgtatham/putty/latest/w64/putty-64bit-0.76-installer.msi)

* Installer la carte SD dans la cible

* Connecter la cible sur la machine de développement via port série-USB

* lancer Putty depuis windows =>

  * __Type de connection__ : Serial

  * __Port__ : COM6

  * __Vitesse de communication__ : 115200 baud

    => `open`

* La cible démare l'image linux qui demande le __login : root__

### Mise en place de l'infrastructure réseau

* On travaille avec des adresses IP fixes du côté de la cible comme du côté de la machine de développement.

  * __Cible : 192.168.0.14__
  * __Dev : 192.168.0.4__

* L'adresse IP de la cible est déjà configurée (vérification : ifconfig => ok)

* Configurer l'adresse IP fixe __de l'adaptateur Ethernet__ sur la machine de développement (windows)

  * Settings => Network & Internet => onglet Status

  * sélectionner l'interface Ethernet (Ethernet 2 dans notre cas) => Properties => IP Settings => Edit

  * configurer une adresse IP manuelle de la façon suivante : 

    ![IP fixe cible](img\ip_fixe_cible.png)

    __NB : __ IPv4 subnet prefix length = 24 = addresse de sous réseau de 24 bits= _255.255.255.0_

* Depuis la machine de développement, vérifier que la connexion fonctionne : `ping 192.168.0.14`

* Se connecter à la cible en SSH : `ssh root@192.168.0.14`

* Une fois connecter, lancer la commande `uname -a` pour connaitre le système d'exploitation de la cible =>

  ```bash
  Linux csel 5.15.21 #1 SMP PREEMPT Fri Feb 25 20:44:19 UTC 2022 aarch64 GNU/Linux
  ```

### Mise en place de l’espace de travail (*workspace*) sous CIFS/SMB

L'objectif ici est que la machine hôte mette à disposition un répertoire qui sera monté et accédé par la cible. Pour le faire de façon automatique il faut depuis la cible :

* Créer un répertoire 

  ````bash
  mkdir -p /workspace
  ````

* Ajouter la ligne suivante à la fin du fichier `/etc/fstab`

  ````bash
  //192.168.0.4/workspace /workspace cifs vers=1.0,username=root,password=toor,port=1445,noserverino
  ````

* Activer la configuration

  ````bash
  mount -a
  ````

### Génération d’applications sur la machine de développement hôte

Pour faire une compilation croisée depuis la machine de développement, le __makefile__ doit faire appeler les bons outils de la __toolchain__ adaptée à la cible (gcc, gdb, ...). Les indications suivantes seront utilisées :

````makefile
# Makefile toolchain part
TOOLCHAIN_PATH=/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-

# Makefile common part
CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AR=$(TOOLCHAIN)ar
CFLAGS+=-Wall -Wextra -g -c -mcpu=cortex-a53 -O0 -MD -std=gnu11
````

### Debugging de l’application sur la cible (VS-Code)

Pour debugger de façon simple, on installe __gdbserver__ sur la cible et __gdb__ (complet) sur la machine hôte. Ceci permet de debugger le programme qui tourne sur la cible depuis VS-Code, sur la machine hôte.

NB : __gdbserver__ prends peu de place et peut dont être installé sur la cible sans problème.

* VS-Code permet de debugger plusieurs projets séparés en ouvrant le même "folder" grâce à son système __multi-root workspaces__

* Pour debugger, il a besoin de trois fichiers :

  * Un fichier “workspace”
  * Un fichier “task.json”
  * Un fichier “launcher.json” 

  NB : les fichiers __task.json__ et __launcher.json__ se trouvent aux racines des répertoires __.vscode__, qui se trouvent eux à la racine de chaque projet.

  Ex : `/workspace/src/fibonacci/.vscode/`

#### Core dumps

les core dumps permettent de visualiser à quel endroit du code le programme sur la cible a crashé. Une portion du code sera enregistrée dans un fichier nommé `core` avec des indications sur le bug..

* Il est nécessaire d'autoriser le core dumps en spécifiant ou non une taille maximale de fichier à ne pas dépasser. Depuis la cible :

	````bash
	ulimit -c <size|unlimited>
	````

	__NB : __ _unlimited_ : pas de limite de taille.
	
* lorsqu'on lance un programme depuis la cible, utiliser core dumps :

  ````bash
  gdb <program-name> <core-file>
  ````

  __NB :__ le core-file se nomme généralement __core__, il faut donner le path complet du fichier

* pour récupérer l'information sur l'hôte :

  ````bash
  sudo <path><target>-gdb <program-name> <core-file>
  ````

ou automatiquement sur vs code (à vérifier...)

### Mise en place de l’environnement pour le développement du noyau sous CIFS/SMB

* Créer un répertoire `boot-scripts` à la racine du workspace et y créer le fichier `boot-cifs.cmd` contenant le code suivant :

  ```
  setenv myip        192.168.0.14
  setenv serverip    192.168.0.4
  setenv netmask     255.255.255.0
  setenv gatewayip   192.168.0.4
  setenv hostname    myhost
  setenv mountpath   rootfs
  setenv bootargs    console=ttyS0,115200 earlyprintk rootdelay=1 root=/dev/cifs rw cifsroot=//$serverip/$mountpath,username=root,password=toor,port=1445 ip=$myip:$serverip:$gatewayip:$netmask:$hostname::off
  
  fatload mmc 0 $kernel_addr_r Image
  fatload mmc 0 $fdt_addr_r nanopi-neo-plus2.dtb
  
  booti $kernel_addr_r - $fdt_addr_r
  ```

* Créer également un fichier `Makefile` dans ce même répertoire contenant :

  ```makefile
  boot.cifs: boot_cifs.cmd
  	mkimage -T script -A arm -C none -d boot_cifs.cmd boot.cifs
  ```

* Depuis le terminal, entrer dans ce répertoire et lancer la commande `make`

  => le fichier `boot.cifs` est alors créé

* Copier ce fichier sur la partition `boot` de la carte SD (en la connectant directement avec un adaptateur USB)

* Redémarrer la cible avec sa carte SD et l'arrêter dans __U-Boot__ en pressant une touche au démarrage

* Depuis __U-Boot__, changer le script de démarrage pour `boot.cifs` et sauvegarder la configuration :

  ````bash
  setenv boot_scripts boot.cifs
  saveenv
  ````

* Redémarrer la cible à l'aide de la commande `boot` depuis U-Boot

* Lors du premier redémarrage, la cible crée une nouvelle paire de clés SSH

  => se connecter sur la console `root` et lancer la commande `chmod go= /etc/ssh/*_key`

  NB : Il faut accepter la nouvelle clé lors de la première connexion

## Questions

1. _Comment faut-il procéder pour générer l’U-Boot ?_

   Le bootloader U-Boot est généré par Buildroot en même temps que le kernel, le rootfs ainsi que la toolbox.

2. _Comment peut-on ajouter et générer un package supplémentaire dans le Buildroot ?_

   Pour générer un package _custom_ dans le Buildroot, il faut créer un nouveau répertoire dans `buildroot/package`. Ce répertoire doit contenir au moins deux fichiers qui sont `myCustomPackage.mk` et `Config.in`. Enfin, il faut référencer ce nouveau package dans le fichier `buildroot/package/Config.in`. Le package est alors disponible dans l'interface de configuration `make menuconfig`, depuis lequel il pourra être ajouter au noyau Linux. 

3. _Comment doit-on procéder pour modifier la configuration du noyau Linux ?_

   On peut modifier la configuration du noyau Linux en exectant la commande `make menuconfig` depuis la racine du répertoire de buildroot. un menu de configuration interactif se lance alors dans le terminal. Il est ensuite nécessaire de recompiler le noyau à l'aide de la commande `make`

4. _Comment faut-il faire pour générer son propre rootfs ?_

   ??

5. _Comment faudrait-il procéder pour utiliser la carte eMMC en lieu et place de la carte SD ?_

   On pourrait sans problème utiliser l'eMMC à la place de la carte SD. En revanche, il faudrait flasher le circuit à distance, via le port série par exemple.

6. _Dans le support de cours, on trouve différentes configurations de  l’environnement de développement. Qu’elle serait la configuration  optimale pour le développement uniquement d’applications en espace utilisateur ?_

   Même si le travail de développement se limite à des applications en espace utilisateur, il serait préférable d'opter pour un environnement de développement où le _rootfs_ de la cible serait partagé par la machine de développement via _nfs_. En effet, cela permettrait d'utiliser un IDE tel que _VS Code_ proposant des outils de debug ergonomiques et de tester les applications en cours de développement. En revanche, le noyau pourrait très bien être stocké dans la mémoire flash de la cible si il n'a pas besoin d'être recompiler pendant le développement.

## Etat d'avancement / compréhension

La première partie de ce laboratoire consistait à se servir de _Buildroot_ pour générer les outils nécessaires à faire fonctionner Linux sur un système embarqué et permettre la compilation croisée depuis la machine de développement pour la cible. Cette partie a été acquise. Dans un deuxième temps, il a fallut configurer l'environnement de développement afin de simplifier le développement d'applications pour la cible. Là encore, les différentes méthodes ainsi que la justification de la direction prise est claire pour les membres du groupe. Enfin, il a été question de tester différentes méthodes de débogage. Cette dernière partie mériterait d'être exercée d'avantage. Celà dit, le mini projet sera l'occasion de le faire.

# 2. Modules Noyaux (du 11.03 au 18.03)

## Résumé

## Questions

## Etat d'avancement / compréhension

## Retour personnel sur le laboratoire

# 3. Pilotes de périphériques (du 25.03 au 01.04)

## Résumé

## Questions

## Etat d'avancement / compréhension

## Retour personnel sur le laboratoire

