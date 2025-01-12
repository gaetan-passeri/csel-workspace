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

### Pilotes orientés mémoire

Les pilotes orientés mémoire permettent de mapper des zones de la mémoire physique du processeur sur une zone de mémoire virtuelle nécessaire au pilotage de périphériques. Ces pilotes se développent en zone utilisateur. Pour ce faire, on emploi l'opération `mmap`, disponible en intégrant la bibliothèque `<sys/mman.h>`.

Une fois la zone mémoire mappée, elle est accessible à l'emplacement `/dev/mem`.

#### Mise en place du pilote

* inclusion de la bibliothèque

  ```c
  #include <sys/mman.h>
  ```

* ouverture du fichier `/dev/mem`

  ```c
  int fd = open("/dev/mem", O_RDWR);
  if (fd < 0) {
      printf("Could not open /dev/mem: error=%i\n", fd);
  	return -1;
  }
  ```

* _mapping_ de la mémoire afin de rendre des registres du processeur accessibles depuis la mémoire virtuelle (les commentaires détaillent les opérations effectuées)

  ```c
  size_t psz     = getpagesize();		// récupération de la taille d'une page mémoire
  off_t dev_addr = 0x01c14200;		// adresse du début des registres que l'on souhaite récupérer
  off_t ofs      = dev_addr % psz;	// nombre d'octets d'écart entre l'adresse de nos registres et le début d'une nouvelle page
  								 // (on souhaite récuperer une page entière!)
  off_t offset   = dev_addr - ofs;	// adresse du début de la page dans laquelle se trouvent ces registres 						
  
  volatile uint32_t* regs = mmap (
      0,					 	// void* addr		=> généralement NULL, adresse de départ en mémoire virtuelle
      psz,					// size_t length	=> taille de la zone à placer en mémoire virtuelle ( => taille d'une page entière)
      PROT_READ | PROT_WRITE,   // int prot		  => droits d’accès à la mémoire: read, write, execute
      MAP_SHARED,				// int flags		 => visibilité de la page pour d’autres processus: shared, private
      fd,	 					// int fd		    => descripteur du fichier correspondant au pilote
      offset					// off_t offset		=> début de la zone mémoire à placer en mémoire virtuelle
  ); 
  ```

* opérations souhaitées sur le périphérique à l'aide de l'adresse virtuelle retournée par `mmap`

  ```c
  // contrôle du pointeur retourné par `mmap`
  if (regs == MAP_FAILED) {  // (void *)-1
      printf("mmap failed, error: %i:%s \n", errno, strerror(errno));
      return -1;
  }
  
  // copie des valeurs en RAM pour utilisation dans le programme
  uint32_t chipid[4] = {
      [0] = *(regs + (ofs + 0x00) / sizeof(uint32_t)),
      [1] = *(regs + (ofs + 0x04) / sizeof(uint32_t)),
      [2] = *(regs + (ofs + 0x08) / sizeof(uint32_t)),
      [3] = *(regs + (ofs + 0x0c) / sizeof(uint32_t)),
  };
  ```

* libération de l'espace mémoire avec la fonction `munmap`

  ```c
  munmap((void*)regs, psz);
  ```

* fermeture du fichier virtuel

  ```c
  close(fd);
  ```

### Pilotes orientés caractères

Un pilote orienté caractères permet d'interagir avec des périphériques de façon assez simple. Il faut faire la distinction entre le pilote, qui est le programme permettant de piloter un ou plusieurs périphériques de même type, et le périphérique ou _device_, qui est une instance du pilote dédiée à la gestion d'un objet matériel. L'échange de données entre l'instance du pilote et le périphérique matériel se fait au travers de fichiers virtuels créés par le pilote.

#### Structure du pilote

* inclusion des bibliothèques nécessaires

  ```c
  #include <linux/cdev.h>        /* needed for char device driver */
  #include <linux/fs.h>          /* needed for device drivers */
  #include <linux/init.h>        /* needed for macros */
  #include <linux/kernel.h>      /* needed for debugging */
  #include <linux/module.h>      /* needed by all modules */
  #include <linux/moduleparam.h> /* needed for module parameters */
  #include <linux/uaccess.h>     /* needed to copy data to/from user */
  ```

* déclaration de l'objet `dev_t` permettant de définir le numéro de pilote

  ```c
  static dev_t skeleton_dev;
  ```

  Le numéro de pilote est constitué de :

  * numéro majeur de 12 bits
  * numéro mineur de 20 bits

  Il est réservé dynamiquement lors du chargement du pilote, à l'aide de la fonction `alloc_chrdev_region`

* déclaration de la structure `skeleton_cdev` permettant l'enregistrement du pilote caractère dans le noyau

  ```c
  static struct cdev skeleton_cdev;
  ```

  Lors du chargement du pilote, la structure `cdev` est :

  * initialisée à l'aide de la méthode `cdev_init`
  * enregistrée dans le noyau à l'aide de la méthode `cdev_add`

* définition de la structure de fichier

  ```c
  static struct file_operations skeleton_fops = {
      .owner   = THIS_MODULE,
      .open    = skeleton_open,
      .read    = skeleton_read,
      .write   = skeleton_write,
      .release = skeleton_release,
  };
  ```

  Chaque attribut que l'on souhaite utiliser dans le pilote doit être initialisé à l'aide d'un pointeur vers une fonction implémentée dans le code du pilote.

* Lors de l'initialisation du module (`skeleton_init`), il faut réserver dynamiquement le numéro du pilote et enregistrer le pilote dans le noyau :

  ```c
  static int __init skeleton_init(void) {
      // Réservation dynamique du numéro de pilote
      int status = alloc_chrdev_region(
          &skeleton_dev,	// instance dev_t
          0,			   // base_minor : premier numéro mineur du pilote
          1,			   // count : le nombre de numéros mineurs requis par le pilote
          "mymodule"	    // nom du pilote de périphérique
      );
      
      if (status == 0) {
          // enregistrement du pilote dans le noyau
          //	=> association entre les numéros majeurs / mineurs et les opérations de fichies attachées au pilote
          cdev_init(&skeleton_cdev, &skeleton_fops);
          skeleton_cdev.owner = THIS_MODULE;
          status = cdev_add(
              &skeleton_cdev,	// pointeur sur la structure du pilote
              skeleton_dev,	// numéro du pilote
              1			   // count : indique le nombre de périphériques
          );
      }
  
      pr_info("Linux module skeleton loaded\n");
      return 0;
  }
  ```

* Lors du déchargement du pilote, il faut éliminer le pilote dans le noyau et libérer les numéros majeur et mineur du pilote.

  ```c
  static void __exit skeleton_exit(void) {
      // élimination du pilote dans le noyau
      cdev_del(&skeleton_cdev);
      // libération des numéros (majeurs/mineurs) du pilote
      unregister_chrdev_region(skeleton_dev, 1);
      pr_info("Linux module skeleton unloaded\n");
  }
  ```

#### Méthode Read

La méthode `skeleton_read` implémentée dans le code du pilote écrit les données nécessaires dans le buffer de l'espace utilisateur. Elle prends en paramètre :

* le descripteur du fichier ouvert dans l'espace utilisateur
* le pointeur vers le buffer de destination
* la quantité d'octets maximale à écrire dans l'espace utilisateur
* offset d'écriture (les _n_ premiers octets avant ceux qu'on envoi à l'utilisateur)

La méthode met à jour la valeur de l'offset afin de ne pas envoyer plusieurs fois les mêmes données à l'utilisateur. Elle retourne ensuite le nombre d'octets qu'elle y a écrit.

```c
static ssize_t skeleton_read(struct file* f, char __user* buf, size_t count, loff_t* off) {
    // compute remaining bytes to copy, update count and pointers
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    
    // init. read ptr
    char* ptr = s_buffer + *off;
    
    // protect reading against buffer overflow
    if (count > remaining) count = remaining;
    
    // compute future return count
    *off += count;
    
    // copy required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT; // EFAULT = bad adress
    // NB : copy_to_user retourne le nombre d'octets qui n'ont pas pu êtres copiés!

    return count; // retourne le nombre d'octets écris dans l'espace utilisateur
}
```

#### Méthode Write

La méthode `skeleton_write` copie les données de l'espace utilisateur vers le buffer instancié dans le code du pilote.

La méthode doit prendre en considération la taille maximale du buffer du pilote afin d'éviter un éventuel débordement. Elle retourne le nombre d'octets copiés dans le buffer.

```c
static ssize_t skeleton_write(struct file* f, const char __user* buf, size_t count, loff_t* off) {

    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    pr_info("skeleton: at%ld\n", (unsigned long)(*off));

    // check if still remaining space to store additional bytes
    if (count >= remaining) count = -EIO;

    if(count > 0){
        char* ptr = s_buffer + *off;
        *off += count;    
        ptr[count] = 0; // make sure string is null terminated
        if(copy_from_user(ptr, buf, count)) count = -EFAULT;
    }

    pr_info("skeleton: write operation... written=%ld\n", count);

    return count; // retourne le nombre d'octets copiés
}
```

#### Création du fichier d'accès au périphérique dans le répertoire `/dev`

Après avoir chargé le pilote, il faut créer le fichier à l'aide de la commande `mknod`

```bash
mknod /dev/mymodule c 511 0
```

* __c__ : type de périphérique caractère
* __511__ : numéro majeur
* __0__ : numéro mineur

#### données privées d'instance

le descripteur de fichier contient un pointeur `private_data` qui peut être initialisé pour pointer sur un espace de données dans le pilote. Ceci permet d'attribuer un espace de donnée propre à chaque instance du pilote.

Pour ce faire, il faut :

* déclarer __en global__ un pointeur de pointeur pour le tableau de buffers

  ```c
  #define BUFFER_SZ 1000
  static char** buffers = 0;
  ```

* définir un nombre d'instance, un paramètre de module peut être utiliser pour rendre ce nombre configurable

  ```c
  static int instances = 3;
  module_param(instances, int, 0);
  ```

* lors de l'initialisation du pilote, créer un tableau de buffers dynamique en fonction du nombre d'instances :

  ```c
  if(status == 0){
      buffers = kzalloc(instances * sizeof(char*), GFP_KERNEL);
      for(i=0; i<instances; i++){
          buffers[i] = kzalloc (BUFFER_SZ, GFP_KERNEL);
      }
  }
  ```

* dans la méthode `open`, il faut initialiser le pointeur `private_data` du descripteur de fichier pour le faire pointer sur le buffer correspondant à l'instance que l'utilisateur a ouvert :

  ```c
  f->private_data = buffers[iminor(i)];
  pr_info("skeleton: private_data=%p\n", f->private_data);
  ```

  __NB__ : la méthode `iminor(struct inode* i)` renvoi le numéro mineur de l'instance (entre _0_ et _instances_)

* lors d'opérations de lecture / écriture, il faut se référer au pointeur configuré pour l'instance lors de l'ouverture du fichier

  ```c
  char* ptr = (char*) f->private_data + *off;
  ```

* enfin, lors du déchargement du module, il faut libérer la mémoire allouée pour le tableau de buffers

  ```c
  for(i=0; i<instances; i++) kfree(buffers[i]);
  kfree(buffers);
  ```

__NB__ : il ne faut pas oublier d'initialiser le nombre correct d'instances dans la structure `dev_t skeleton_dev` lors du chargement et du déchargement du module.

### Sysfs

le sysfs est un système de fichiers virtuels permettant de représenter les objets du noyau dont les périphériques. Il est accessible via le répertoire `/sys`.

L'objectif de cette dernière partie est de construire des pilotes sous forme de classes de périphériques qui seront accessibles dans `/sys/class`.

#### Structure du pilote

* Pour chaque attribut que l'on souhaite présenter dans le _sysfs_, il faut :

  * déclarer l'attribut en tant que __variable globale__ dans le code du pilote. Les attributs peuvent êtres de n'importe quel type, y compris des structures. Ex :

    ```c
    struct skeleton_config {
        int id;
        long ref;
        char name[30];
        char descr[30];
    };
    static struct skeleton_config config;
    ```

  * créer une méthode d'accès en lecture / présentation dans le _sysfs_. Cette méthode retournera le contenu à afficher lorsqu'une commande `cat` sera effectuée sur le fichier correspondant à l'attribut.

    ```c
    ssize_t sysfs_show_cfg(struct device* dev, struct device_attribute* attr, char* buf){
        sprintf(
            buf, "%d %ld %s %s\n",
            config.id,
            config.ref,
            config.name,
            config.descr
        );
        return strlen(buf);
    }
    ```

    La méthode copie le contenu à afficher dans le buffer pointé en argument et en retourne la taille en octets.

  * créer une méthode d'accès en écriture (store). Cette méthode prendra en argument un pointeur vers un buffer contenant la donnée à stocker dans l'attribut.

    ```c
    ssize_t sysfs_store_cfg(struct device* dev, struct device_attribute* attr, const char* buf, size_t count){
        sscanf(
            buf,            // src string
            "%d %ld %s %s",  // string data format
            &config.id,     // pointers to data
            &config.ref,    // |
            config.name,    // |
            config.descr    // |
        );
        return count;
    }
    ```

    La méthode copie le contenu du buffer pointé en argument dans l'attribut. Dans l'exemple ci-dessus, l'attribut étant une structure, la méthode `sscanf` est utilisée afin de respecter la structure des données.

  * utiliser la macro `DEVICE_ATTR` pour instancier la structure `device_attribute` qui spécifie les méthodes d'accès de l'attribut.

    ```c
    DEVICE_ATTR(config, 0664, sysfs_show_cfg, sysfs_store_cfg);
    
    ```

    la structure `device_attribute` contient les éléments suivants:

    ```c
    struct device_attribute {
    	struct attribute attr;
    	ssize_t (*show) (struct device *dev, struct device_attribute *attr, char *buf);
    	ssize_t (*store) (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
    };
    ```

    Les derniers arguments sont des pointeurs vers les méthodes implémentées plus haut.

* en global, il faut déclarer les deux structures suivantes représentant respectivement la classe et le device :

  * structure de classe `sysfs_class`

      ```c
      static struct class* sysfs_class;
      ```

  * structure device `sysfs_device`

      ```c
      static struct device* sysfs_device;
      ```

* lors du chargement du module (dans la méthode `skeleton_init`), il faut :

  * instancier la classe préalablement déclarée à l'aide de la méthode `class_create`

    ```c
    sysfs_class  = class_create(
        THIS_MODULE,	// struct module * owner => le propriétaire de la classe 
        "my_sysfs_class"// const char * name	 => le nom de la classe
    );
    ```

  * instancier le _device_ préalablement déclaré à l'aide de la méthode `device_create`

    ```c
    sysfs_device = device_create(
        sysfs_class,        // structure classe
        NULL,               // device parent
        0,                  // dev_t devt (définition du numéro de pilote)
        NULL,               // format (const char *)
        "my_sysfs_device"   // nom du device
    );
    ```

  * Installer les méthodes d'accès pour chaque attribut
  
    ```c
    if (status == 0) status = device_create_file(
        sysfs_device,       // device
        &dev_attr_config    // device_attribute struct address
    );
    ```
  
* Lors du déchargement du module (dans la méthode `skeleton_exit`), il faut :

  * désinstaller les méthodes d'accès pour chaque attribut

    ```c
    device_remove_file(sysfs_device, &dev_attr_config);
    ```

  * détruire le _device_

    ```c
    device_destroy(sysfs_class, 0);
    ```

  * détruire la classe

    ```c
    class_destroy(sysfs_class);
    ```



## Questions

## Etat d'avancement / compréhension

## Retour personnel sur le laboratoire

