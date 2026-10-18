> Jalon historique : la politique HD et le filtrage actuels sont décrits dans [VALIDATION-UNIFORME.md](VALIDATION-UNIFORME.md).

# Validation hybride HD-2D — 18 octobre 2026

## Périmètre livré

- Inventaire décodé : **117 BMP / 2 392 sprites**. Tout sprite non remplacé passe par l’atlas R8 et la LUT/filtre GPU en mode moderne, indépendamment de sa famille.
- Sept planches Johnny validées / 84 correspondances explicites ; décor et palmier conservés, soit neuf grandes textures RGBA potentielles. Les quatre planches pêche demeurent archivées, sans chargement GPU. `active-retro.json` distingue actifs et archives.
- Aucun nouveau PNG/RGBA d’art n’a été généré. Les 62 PNG/JCR préexistants sont identiques aux sommes du jalon précédent. Les nouveaux PNG sont des captures de validation, pas des planches.
- `vm_core.c`, `engine.c`, `gfx.c`, `ttm.c`, `island.c`, `resource.c` : SHA avant/après identiques. `render_frame.c` ne change que la rétention du radeau dans la liste moderne ; le canvas et le flux classique restent inchangés.

## Staging : allocation demandée, acceptée et observée

Sokol utilisait déjà un buffer de copie séquentiel pour les images immuables (`_sg_vk_staging_copy_miplevel_data` dans le header fourni). Le réglage de 64 Mio concernait les ressources `write_transient`, notamment l’atlas, et non neuf décodages RGBA simultanés.

| Capacité de transfert | Avant | Hybride |
|---|---:|---:|
| Buffer de copie synchrone partagé | 4 Mio | 1 Mio |
| Buffer transitoire par image en vol | 64 Mio | 2 Mio |
| Images en vol dans ce backend | 2 | 2 |
| Relais atlas | 8 Mio | 2 Mio |
| Total de staging | **140 Mio** | **7 Mio** |

Valeurs relues avec `sg_query_desc()` : `1048576 copy bytes + 2097152 stream bytes x 2 frames`. Relais relu avec `sg_query_buffer_desc()` : `2097152 bytes`. GPU relu : `AMD Radeon 780M Graphics (RADV PHOENIX)`.

Le relais atlas est passé de 8 à 2 Mio et il est inclus dans le total. Cette voie garde les uploads au rythme des nouveaux instantanés VM. Une image `write_transient` directe imposerait une écriture à chaque image de rendu ; cette variante n’est pas celle livrée.

Les planches sont chargées avant le début d’une passe, à leur première utilisation. Le JCR décode au plus une planche à la fois (6 Mio pour une planche Johnny), et les pixels CPU sont libérés après `sg_make_image`. Le buffer de copie de 1 Mio est réutilisé, pas réalloué pour chaque texture. Il reste réservé dans Sokol. Les planches utilisées restent en cache ; le système ne promet pas une éviction LRU.

Maximum observé sur les 79 scènes, graine4242 : **814 lignes d’atlas, soit 1 667 072 octets**. Si un atlas dépasse 2 Mio, une image immuable est initialisée par morceaux via le buffer de copie de 1 Mio. Le retour vers une image `copy_dst` est géré quand le flux redevient petit. Aucun flag d’image CPU-writable n’est combiné à des données d’initialisation immuables.

Le chemin exceptionnel a été forcé avec `JC_STREAM_MIB=1`, scène32 au pas100 : 240 images présentées, sortie0 et aucune erreur de validation. Ce test serait insuffisant si seul le budget normal était vérifié.

## Mesure mémoire sur machine réelle

Trois paires d’essais alternés avant/hybride, même scène51, départ au pas50, graine4242, horloge1993-06-01T10:00, 240 images présentées. Échantillonnage toutes les10 ms depuis le lancement jusqu’à la sortie, environ4 secondes : pic de démarrage **et des premières actions**, pas uniquement le constructeur. Les pics DRM sont lus dans `/proc/PID/fdinfo`, avec déduplication des clients DRM ; le RSS via `VmHWM`. Le JSON brut est fourni dans `validation/hybrid-memory.json`.

| Médiane des trois pics | Avant | Hybride |
|---|---:|---:|
| Mémoire GPU GTT du processus | 142,71 Mio | 15,71 Mio |
| Mémoire GPU VRAM du processus | 135,62 Mio | 75,62 Mio |
| RSS CPU | 62,40 Mio | 62,56 Mio |


La baisse GTT mesurée est de **127 Mio**. Le relais de transfert est en mémoire device-local : sa réduction de8 à2 Mio apparaît côté VRAM, pas dans le GTT. Au total, la capacité de transfert baisse de133 Mio (140→7), répartis entre127 Mio côté GTT et6 Mio côté device-local. La baisse VRAM inclut les planches chargées à la demande : seules deux planches Johnny sont utilisées dans ce parcours, contre onze chargées d’avance dans le binaire précédent. Les allocations du pilote et leur résidence font varier les pics VRAM. Il ne faut pas additionner ces métriques à un RSS puis les présenter comme une mesure unique de RAM physique sur une machine UMA.

**Le RSS CPU reste essentiellement stable.** Cette mesure démontre une baisse des allocations GPU/transfert, pas une baisse de133 Mio du RSS CPU. Elle ne démontre pas une économie d’énergie ni une cadence soutenue de144 FPS. Le cache peut atteindre les sept planches sur un scénario long.

## Filtre et éclairage

`shaders/palette.glsl` récupère le voisinage3×3 avec `texelFetch`, borné au rectangle du sprite dans l’atlas. La règle de coins inspirée de Scale2x et une couverture diagonale continue préservent les aplats. Ce n’est pas l’algorithme complet ScaleFX/xBR et il n’y a pas de flou bilinéaire de l’atlas.

La LUT moderne est calculée en mémoire à partir de la palette originale (256 entrées RGBA, 1 Kio). Le mode classique conserve sa palette. Les contours/différences d’index donnent un pseudo-relief léger. La teinte ambiante, la direction du soleil/de la lune et l’intensité du rim-light sont partagées avec Johnny HD, le palmier et le décor. Les normales des sprites retournés suivent le miroir. Les glyphes vectoriels restent lisibles avec leur coloration plate.

L’horloge locale possède une composante seconde/fraction ; l’horloge injectée donne un éclairage fixe reproductible. Les transitions et la périodicité de la fonction d’éclairage sont testées. Le décor demeure une image de coucher de soleil recolorée ; il n’y a pas de ciel physique ni de nouvelle illustration nocturne.

## Ombres

Passe avant les acteurs, alpha de la cellule HD ou masque d’index non nul R8. Corps verticaux : ligne de base visible, écrasement0,35, cisaillement horizontal lié à l’heure. Le padding transparent du BMP ne décale pas l’ombre. Le radeau posé à plat utilise une projection de contact sur son plan afin de ne pas sembler flotter. Couleur15/20/35, alpha diurne0,32, réduit et adouci la nuit.

Le masque sable intersecte une ellipse conservatrice de l’île et les pixels chauds du décor JCR, avant son éclairage. Les objets hors de cette zone ne projettent pas systématiquement d’ombre. Les fragments déjà identifiés comme torse, tête, main, symbole ou éclaboussure ne créent pas une deuxième ombre corporelle ; les jambes porteuses conservent celle du personnage multipartite.

Le masque n’est pas une segmentation exacte de chaque marée, ni une carte de profondeur. La classification au sol des familles encore inconnues reste heuristique. Le masque d’occultation du palmier et les clips des acteurs sont conservés.

## Couverture et compositions spéciales

Le garde historique « nuit ou0x400 = classique » ne coupe plus systématiquement le décor moderne. Sur le parcours inspecté (graine4242, 17:00), les scènes69,71,72,73 ainsi qu’une partie de10 à partir du pas582 gardent la **composition originale entière harmonisée**. Ces images conservent leurs copies/effacements GDI ; elles ne reçoivent pas des remplacements HD ni des ombres individuelles d’objets à travers le canvas aplati.

Les autres scènes peuvent employer les couches modernes capturées, dont le radeau précédemment omis. D’autres graines et branches peuvent changer la répartition. L’harmonisation couvre les79 scènes, mais cela ne signifie pas que79 scènes sont entièrement décomposées, éclairées et réillustrées en HD.

## Tests

- Compilation de l’application et des tests : C99, `-Wall -Wextra -Wpedantic -Werror`.
- `make test` et **`make sanitize` complets** réussis (ASan/UBSan, `detect_leaks=0` comme précédemment).
- Reconstruction classique exacte : **79 scènes / 30 537 images**, zéro dépassement d’atlas.
- Tests ressources, VM, INI, capture, marche, lecture et données pêche historiques maintenus. Le test de marche accepte désormais le radeau dans la liste moderne et vérifie toujours les occultations.
- `tests/test_hybrid.c` : inventaire2392,84 correspondances HD retenues, sélection R8 pour la pêche, cycle continu/périodique, exclusion de l’eau, rétention du radeau et canvas inchangé.
- Huit essais Vulkan debug de240 images : classique9 ; R8 seul9 ; mixte51/jour6 ; construction3 ; lecture34 ; nuit51 ; composition spéciale71 ; petit staging32. Tous : Radeon780M, sortie0, aucun message de validation Vulkan, Sokol error ou panic. Seul avertissement DPI53 déjà connu.
- Binaire normal et compact :1000 pas headless `--verify`, graine1, horloge2026-10-18T12:00 ; hash commun `de8bfcfeb48c1c6b`, aucun repli d’atlas. Compact également vérifié en plein écran avec validation Vulkan.

Les journaux et la mesure brute sont dans `validation/`. ELF livré : **214 648 octets** ; compact UPX : **83 996 octets**, hors ressources et bibliothèques. Le lien ELF suit désormais la disposition standard du linker : le réglage forcé `-z noseparate-code` déclenchait un arrêt dans le loader UPX pour cette taille de programme. Le compact a été retesté après correction ; le simple succès de `upx -t` ne suffit pas à valider son exécution.

## Capture comparative

`preview-hybrid.png` : deux fenêtres capturées séparément, même scène51 / jour6 / pas50 / 17:00, eau figée dans les exécutables de capture. La référence ne désactive que le traitement couleur/filtre du R8 ; Johnny HD et les ombres sont communs. La différence est confinée au rectangle du radeau. Les deux vues sont assemblées et légendées, sans retoucher leurs pixels. Le programme livré conserve sa VM et son eau animées.

`preview-hybrid-scene.png` conserve la vue hybride seule ; `preview-hybrid-night.png` provient de la fenêtre nocturne. Aucun de ces fichiers n’est un nouvel asset d’art.
