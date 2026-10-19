# Décor actif

`island-registered-v2.jcr` est le seul asset artistique chargé par le mode
moderne. Il contient une illustration créée avec imagegen et recalée sur la
géométrie du jeu original ; il ne provient pas des ressources Sierra.

JCR1 : signature ASCII, largeur/hauteur uint32 little endian, pixels RGBA8 sans
compression. Le PNG source contenait les mêmes pixels ; il a été supprimé
comme doublon, sans réduire la qualité du décor chargé par le moteur.
Le palmier, Johnny et les accessoires sont issus des sprites originaux R8.
Les masques du rivage et les effets animés sont calculés à l’exécution.

Les anciennes variantes sunset-v1, planches de personnages et textures du
palmier peint ne sont plus distribuées. Le code de calage est dans layout.h
et palm_layout.h. Un décor personnalisé doit respecter ces ancrages.
