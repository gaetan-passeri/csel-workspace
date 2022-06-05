### Ex01 results

    234      context-switches          #    5.852 /sec  
    1673821036      instructions              #    0.05  insn per cycle 
    40.039115853 seconds time elapsed
    
    39.264146000 seconds user
    0.288091000 seconds sys

#### Correction

Les cache-misses sont dû à la boucle for K. 
C'est pourquoi cette boucle a été enlevé et remplacer par Array += 10;

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

On peut voir que le nomnbre de cache-misses et le temps d'exécution ont été divisé par 10.

#### Définition 
Instruction: Le nombre d'insctruction processeur que le programme a effectué

Cach-misses: Le nombre de donnée qui n'était pas stocké dans la cach 
    
branch-misses: Le saut 
    
L1-dcache-load-misses: 
    
cpu-migrations: 

context-switches: Changement de contexte 

#### Mesure de l’impact sur la performance

Le résultats du temps d'exécution avec time est de 35.6 secondes
Le temps d'exécution donnée par perf stat + time est de 35.9 secondes

On peut voir que les fonction time n'affecte que très peu le temsp d'exécution.

### Ex02 


#### Explication code ex02

Première étape un tableau de short contenant 256 indice est déclaré, en suite le tableau est initialisé avec des valeurs alléatoire comprise entre 0 et 251.

Deuxième étape calcul une somme. Pour cela deux boucles sont implémentées. La première de 1000 itération et la deuxième de 256. La somme n'est calculé qu'avec des nombres plus grand que 256

Finalement cette somme est affiché à l'utilisateur 

#### Mesure du temps d'exécution 

Le temps d'exécution mesuré avec perf stat est de 26.2 secondes.

#### Optimisation 

Le temps d'exécusion obtenu en ayant apporté les optimisation est de 23.4 secondes. (réalisé avec perf stat)

#### Mesure 

Le programme s'exécute plus rapidement, car le tableau est trié. En effet, la condition à l'intérieur des boucles for ralenti l'exécution du programme. Cela est dû au fait que le programme ne peut pas prédire la prochaine étape. 
Losque le tableau est trié, le programme peut prédire la suite, car toute les valeurs inférieures à 256 sont traitées en premieres. 

### Ex03

#### Recherche de lenteur

26.75%  read-apache-log  read-apache-logs       [.] std::operator==<char>                                                 
std::operator==<char>                                                                                           

__gnu_cxx::__ops::_Iter_equals_val<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const>▒
std::__find_if<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>▒
std::__find_if<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>▒
std::find<__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, 
HostCounter::isNewHost                                                                                                   
HostCounter::notifyHost                                                                                                  
ApacheAccessLogAnalyzer::processFile   

La fonction std::operator==<char> est utilisé dans les fonctions HostCounter::isNewHost, HostCounter::notifyHost ainsi que ApacheAccessLogAnalyzer::processFile

#### 