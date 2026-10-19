# Taille de la distribution — 19 octobre 2026

Somme des octets des fichiers distribués, sans compter la taille des dossiers,
les objets Git ni les bibliothèques système. 1 Mio = 1 048 576 octets.

| Périmètre | Octets | Mio |
| --- | ---: | ---: |
| Dépôt courant avant ce nettoyage, sans .git | 105687778 | 100.79 |
| Dépôt source allégé, vidéo de présentation comprise | 12737950 | 12.15 |
| Gain sur le dépôt courant | 92949828 | 88.64 |
| Paquet minimal d’exécution, sans données originales | 6563271 | 6.26 |

**Réduction du contenu distribué : 87.95 %.**
Le paquet runtime est un périmètre différent : il ne fournit ni compilation
ni tests ni vidéo, uniquement l’exécutable standard, le décor actif, les
notices et les deux lanceurs jour/nuit.

## Retiré

238 anciens fichiers : captures, GIF et vidéos intermédiaires,
ancien fond sunset-v1, copie PNG du décor actif, références de calage en images,
métadonnées et outils de planches Johnny supprimées, anciens journaux et
rapports, shader de personnage RGBA non référencé et double binaire compact.
Les planches PNG/JCR de Johnny et du palmier avaient déjà été retirées au
nettoyage précédent : 176 422 276 octets supplémentaires, soit 168,25 Mio.
Aucune qualité de texture active ni fonctionnalité du jeu n’est réduite.

## Téléchargement allégé

Le ZIP de la branche courante et l’archive source locale ne contiennent que
les fichiers actuels. Les anciens gros médias restent dans l’historique Git.
Un clonage complet peut donc rester volumineux ; utiliser :

```sh
git clone --depth 1 https://github.com/Nide96/johnny-hd2d.git
```

Les fichiers RESOURCE.MAP, RESOURCE.001 et SCRANTIC.SCR sont à fournir depuis
sa propre copie du jeu ; ils ne sont dans aucune archive publique.
Ils ajoutent 1 473 058 octets (environ 1,40 Mio) à une installation locale.
SCRANTIC.INI est créé pour la progression normale, puis varie avec l’usage.
Les fichiers work/ et __pycache__/ n’ont aucune utilité dans la distribution.
