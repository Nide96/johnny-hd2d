> Jalon historique. Depuis l’architecture hybride, ces planches de pêche sont archivées ; le rendu utilise le fallback R8. Voir `VALIDATION-HYBRIDE.md`.

# Lot pêche — 18 octobre 2026

## Livraison

Quatre planches actives 1536×1024 RGBA, générées avec l’outil intégré imagegen puis mesurées et empaquetées sans retoucher leurs pixels : `fishing-v1` (16 cellules), `fishing-parts-v2` (14), `fishing-reactions-v1` (12), `fishing-reaction-parts-v1` (15). Le brouillon de fragments v1 reste conservé. Prompts exacts et références : `assets/characters/FISHING-V1.md`.

**57 dessins générés, 54 activés.** MJFISH1 19,28,29 gardent les sprites originaux, car les gestes/fragments générés demandent une reprise. Les corps partiels ne deviennent jamais des corps entiers. Les ombres sont désactivées pour torses, têtes, mains, lignes et vaguelettes ; seules les jambes porteuses et corps entiers en reçoivent.

Les animations gardent les scripts TTM, cadence, positions, miroirs, découpages et ordre de dessin. `generated/fishing_bounds.h` contient seulement les empreintes visibles des 57 sprites originaux. Elles sont réfléchies dans le cadre logique complet avant projection : le padding de stockage ne grossit pas les petits fragments.

## Vérifications du lot final

- Les onze planches actives ont des cellules non vides, des silhouettes non coupées et des gouttières transparentes au seuil alpha128. Les JCR1 reproduisent exactement les RGBA des PNG ; seuls UV et métadonnées sont calculés.
- Compilation C99 réussie ; suite stricte complète réussie, dont **79 scènes / 30 537 images** exactement reconstruites depuis les commandes de présentation. Aucun dépassement d’atlas.
- Test spécifique pêche : 57 empreintes vérifiées contre les pixels décodés pour les quatre combinaisons de retournement ; 54 correspondances et trois exclusions. Six parcours originaux (9,20,21,24,25,27), **2 526 images / 4 339 commandes HD candidates / 39 indices rencontrés**, canvas classique exact. Les 54 cellules actives ne sont donc pas toutes rencontrées par ces branches.
- Ce test pêche passe sous **AddressSanitizer + UndefinedBehaviorSanitizer** ; pas de relance complète de la suite sous sanitizers pour ce lot.
- Quatre essais natifs de **240 images soumises chacun**, sortie0 : scène20 miroir au pas85 (les quatre planches), scène25 au pas45, régression lecture34 au pas135, compact plein écran scène9 au pas100. Validation Vulkan activée, aucun message d’erreur de validation ou de Sokol. Seul avertissement DPI53 connu.
- Capture réelle de notre fenêtre : scène9, graine4242, horloge1993-06-01T10:00, départ au pas45, 152 captures sur19 secondes à8 images/s. Sortie0. Les compteurs GPU des nouvelles planches sont non nuls : 632 / 1733 / 329 / 425 commandes dans cette capture. Les fichiers `preview-fishing-v1.png/.gif/.mp4` en proviennent ; ils ne mesurent pas les FPS du renderer.
- ELF lié dynamiquement : **185 976 octets**, UPX : **76 828 octets**, hors ressources et bibliothèques. Onze planches + fond + palmier =13 textures RGBA, plus atlas indexé et palette. Le staging transitoire reste64 Mio ; initialisation réussie avec validation active.

- Contrôle headless final : normal et UPX, 1 000 pas chacun, `--verify`, graine1, horloge2026-10-18T12:00 : hash identique `de8bfcfeb48c1c6b`, zéro saturation, capture exacte.

## Portée

138 dessins bitmap actifs et deux symboles vectoriels dans le projet. L’inventaire rencontre au moins un remplacement dans58/79 parcours, sans affirmer qu’ils sont entièrement modernes. Les neuf variantes de pêche de l’inventaire sont9,20..27 ; 22,23,26 conservent leur repli classique. Les variantes n’emploient pas toutes les mêmes poses.

Les contacts des illustrations restent approximatifs ; la revue visuelle actuelle porte surtout sur9,20,25. MJFISH3 (poissons/prises/objets), les autres personnages et les autres familles d’action restent d’origine. Aucun 144 FPS soutenu mesuré. Nuit et scènes incompatibles gardent leur repli classique. Les PNG de pêche n’ont pas modifié le décor ni le palmier corrigé au jalon précédent.

## Essai

```sh
./preview-fishing-hd.sh 20 --fullscreen
./preview-fishing-hd.sh 9
./preview-fishing-hd.sh 25
```

Fermer une ancienne fenêtre puis relancer pour charger le nouveau binaire et les nouvelles planches. Échap ferme la fenêtre ; l’horloge fixe empêche la sauvegarde de l’INI réelle.
