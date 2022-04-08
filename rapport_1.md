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

    ![IP fixe cible](img/ip_fixe_cible.png)

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

Ce laboratoire est constitué de 8 exercices. Ces exercices nous permettent de nous familiariser avec plusieurs aspects des modules noyaux. 

1. Exercice 01 : Cet exercice nous permet de voir la structure basique d'un module ainsi que son makefile associé

   1. Un module a besoin, au minimum, d'une fonction **Init** qui sera exécutée lors de son chargement dans le noyau de ce dernier et d'une fonction **exit** qui, elle, sera exécutée au moment de sa désinstallation. 

   2. La commande modinfo permet d'extraire les métadonnées du module.

   3. Voici les messages que nous voyons après installation et désinstallation du module :

      ````bash
      [  704.967401] Linux module 01 loaded
      [  710.109996] Linux module skeleton unloaded
      ````

   4. Les deux commandes nous affichent tous les modules installés sur la machine.

   6. Pour permettre l'installation d'un module avec la commande modprobe, il faut ajouter les deux lignes suivante au Makefile :  

      ```` makefile
      MODPATH := $(HOME)/workspace/buildroot/output/target # production mode install:
      $(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
      ````

2. Exercice 02 : 

   *  Pour pouvoir passer des paramètres au module, il faut ajouter les lignes suivantes :

     ````c
     #include <linux/moduleparam.h>
     static char* text= "dummy help"; 
     module_param(text, charp, 0);
     static int elements= 1; module_param(elements, int, 0);
     ````

     La librairie <linux/moduleparam.h> donne accès à la macro **module_param**. Cette macro permet de définir les paramètres du module.

   * Pour passer des paramètres au module lors de l'installation : 

     ````bash
     insmod mymodule.ko elements=-1 'text="bonjour le monde"'
     ````

3. Exercice 03 :

   ````bash
   cat /proc/sys/kernel/printk
   7		4		1		7
   ````

   Cette commande affiche les niveaux de log courant de la console. Le tableau ci-dessous nous montre la signification des ces valeurs : 

   | NOM          | String | Function    |
   | ------------ | :----- | ----------- |
   | KERN_EMERG   | 0      | pr_emerg()  |
   | KERN_ALERT   | 1      | pr_alert()  |
   | KERN_CRIT    | 2      | pr_crit()   |
   | KERN_ERR     | 3      | pr_err()    |
   | KERN_WARNING | 4      | pr_warn()   |
   | KERN_NOTICE  | 5      | pr_notice() |
   | KERN_INFO    | 6      | pr_info()   |
   | KERN_DEBUG   | 7      | pr_debug()  |

   Pour modifier ces valeurs, on peut exécuter la commande suivante : 

   ````bash
   echo 8 > /proc/sys/kernel/printk
   cat /proc/sys/kernel/printk
   8		4		1		7
   ````

4. Exercice 04 :

   Le code suivant permet de créer une liste : 

   ````c
   static LIST_HEAD (my_list);
   ````

   La structure suivante permet de stocker son numéro unique (id), le texte (str), ainsi que le lien vers le prochain élément de la liste : 

   ````c
   struct element
   {
   	char *str;
   	int id;
   	struct list_head node;
   };
   ````

   Le code suivant permet l'allocation mémoire dynamique des éléments ainsi que son insertion dans la liste : 

   ````c
   for(i;i < elements;i++){
   		struct element* ele;
   		ele = kmalloc(sizeof(*ele), GFP_KERNEL); // create a new element if (ele != NULL)
   		ele->id = i;
   		ele->str = text;
   		list_add_tail(&ele->node, &my_list); // add element at the end of the list }
   		pr_debug(" element with id %d and str %s has been added to the list",ele->id,ele->str);
   	}
   ````

   La libération mémoire des éléments se fait avec le code ci-dessous : 

   ````c
   while(!list_empty(&my_list)){
   		ele = list_entry(my_list.next, struct element, node);
   		list_del(&ele->node);
   		kfree(ele);
   		pr_debug("element free");
   	} 
   ````

5. Exercice 05 : 

   Pour pouvoir accéder aux registres d'un périphérique il faut demander l'accès mémoire à une région au noyeau linux. Pour cela il faut utiliser la fonction suivante : 

   ````c
   res[0] = request_mem_region(CHIP_ID_ADD, 0x1000, "allwiner sid");
   res[1] = request_mem_region(TEMP_CPU_ADD, 0x1000, "allwiner h5 ths");	
   res[2] = request_mem_region(MACC_ADD, 0x1000, "allwiner h5 emac");
   ````

   Une fois cette demande effectuée il faut transférer les adresses mémoire du noyau sur la mémoire virtuel.

   ````c
   reg_chipid = ioremap(CHIP_ID_ADD, 0x1000);
   reg_temp_sensor = ioremap(TEMP_CPU_ADD, 0x1000);
   reg_mac = ioremap(MACC_ADD, 0x1000);
   ````

   Une fois avoir mappé les addresses dans la mémoire virtuel, il est possible de lire et écrire à ces addresses. Dans ce labo on ne fait que de la lecture.

   ````c
   chipid[0] = ioread32(reg_chipid+0x200);
   chipid[1] = ioread32(reg_chipid+0x204);
   chipid[2] = ioread32(reg_chipid+0x208);
   chipid[3] = ioread32(reg_chipid+0x20c);
   ````

   Dans l'exemple au-dessus on vient lire les 4 registres du Chip ID.

   Une fois que on a fini d'utiliser les registres périphérique il faut libérer les espaces mémoire. Cela se fait lors de la désinstallation du module de la manière suivante : 

   ````c
   release_mem_region(CHIP_ID_ADD, 0x1000);
   release_mem_region(TEMP_CPU_ADD, 0x1000);
   release_mem_region(MACC_ADD, 0x1000);
   ````

6. Exercice 06 :

   L'instanciation d'un thread se fait de la manière suivante :

   ````c
   kthread_run(thread, NULL, "EX06");
   ````

   La fonction exécutée dans le thread est la suivante : 

   ````c
   static int thread(void* data){
       while (!kthread_should_stop())
       {
           pr_info("See you in 5 sec");
           ssleep(5);
       }
       return 0;
   }
   ````

   L'arrêt du thread se fait à la désinstallation du module de la façon suivante :

   ````c
    kthread_stop(k);
   ````

7. Exercice 07 :

   Dans cette exercice, l'utilisation des waitqueues était obligatoire. Pour cela, deux waitqueues sont utilisées. La première est initialisée statiquement, tandis que la deuxième est initialisée dynamiquement. 

   La première waitqueues est utilisée par le premier thread pour attendre son réveil par le deuxième thread.

   ````c
   DECLARE_WAIT_QUEUE_HEAD(queue);
   
   static int thread(void* data){
   
       pr_info("Thread 1 is active");
       while (!kthread_should_stop())
       {
           pr_info("Thread 1: is waiting for thread 2");
           wait_event(queue, atomic_read(&start) != 0 || kthread_should_stop());
           
           pr_info("Thread 1: is active");
           atomic_set(&start,0);
       }
       return 0;
   }
   ````

   ​	**NB :** L'utilisation du variable atomic est requise, car les deux threads accèdent à cette dernière. 

   ​      La deuxième waitqueues est utilisée en mode timeout pour se réveiller toute les 5 secondes.

   ````c
   static int thread2(void* data){
   
       wait_queue_head_t queue_2;
       init_waitqueue_head(&queue_2);
   
       pr_info("Thread 2: is active");
       while (!kthread_should_stop())
       {
           int ret = wait_event_timeout(queue, kthread_should_stop(), 5*HZ);
           if(ret == 0){
               pr_info("Thread 2: Timeout elapsed");
           }
           atomic_set(&start,1);
           wake_up(&queue);
           pr_info("Thread 2: See you in 5 sec");
       }
       return 0;
   }
   ````

8. Exercice 08 :

   Dans cet exercice, il était demandé de capturer l'interruption des trois switches de la carte d'extension. 

   Tout d'abord, nous devons obtenir le port GPIO de la manière suivante : 

   ````c
   int status;
   status = gpio_request(K1,k1_label);
   status = gpio_request(K2,k2_label);
   status = gpio_request(K3,k3_label);
   ````

   Une fois le port récupéré, nous devons attacher la fonction d'interruption sur le bon port. Pour cela, il faut obtenir le vecteur d'interruption, puis attacher la callback à ce vecteur tout en spécifiant le fanion de gestion des interruptions. 

   ````c
   request_irq(gpio_to_irq(K1) ,gpio_callback, IRQF_TRIGGER_FALLING, k1_label, k1_label);
   request_irq(gpio_to_irq(K2) ,gpio_callback, IRQF_TRIGGER_FALLING, k2_label, k2_label);
   request_irq(gpio_to_irq(K3) ,gpio_callback, IRQF_TRIGGER_FALLING, k3_label, k3_label);
   ````

   La callback d'interruption ne fait qu'afficher le switche sur lequel l'appuis a été fait.

   ````c
   irqreturn_t gpio_callback(int irq, void *dev_id){
       
       pr_info("interrupt %s: rise",(char*) dev_id);
       
       return IRQ_HANDLED;
   }
   ````

## Questions

Ce laboratoire ne possédait aucune question en plus des exercices vu dans la section précédente.

## Etat d'avancement / compréhension

L'utilisation et l'implémentation des modules ont été parfaitement comprises par le groupe. Les notions de multi threading, mise en sommeil à l'aide de waitqueues et listes d'éléments,  ont été parfaitement comprises. Cependant, la lecture de registre (exercice 5) devra être plus approfondie de notre côté. 

## Retour personnel sur le laboratoire

Les exercices proposés lors de ce laboratoire étaient très intéressants. Avoir la correction rapidement nous a permis d'améliorer notre compréhension des différents sujets abordés. 

# 3. Pilotes de périphériques (du 25.03 au 01.04)

## Résumé

## Questions

## Etat d'avancement / compréhension

## Retour personnel sur le laboratoire

