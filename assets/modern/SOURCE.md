# Décor de prévisualisation — 18 octobre 2026

`island-sunset-v1.png` est un décor généré avec l’outil intégré **imagegen**, par
édition du concept choisi `outputs/direction-artistique/johnny-castaway-3d-lisibilite-v3.png`.
Le concept n’a pas été remplacé. Il ne s’agit pas d’un asset Sierra ni d’une
capture d’une scène calculée en 3D. Johnny, sa canne et le radeau ont été retirés
pour que le personnage soit affiché par la VM.

L’image PNG est conservée comme source. Le moteur charge
`island-sunset-v1.jcr` : signature ASCII `JCR1`, largeur et hauteur uint32 little
endian, puis pixels RGBA8 rangée par rangée, sans compression. Ce format évite
une dépendance de décodage d’image dans le binaire. La conversion ne retouche
pas l’image :

```sh
python3 tools/pack_plate.py assets/modern/island-sunset-v1.png assets/modern/island-sunset-v1.jcr
```

Python/Pillow sont nécessaires seulement pour cette conversion, pas pour lancer
ou compiler le moteur. Le PNG et le fichier JCR contiennent des millions d’octets
externes ; ils ne sont pas inclus dans la cible de taille de l’exécutable.

Prompt exact envoyé à l’outil intégré :

> Use case: precise-object-edit. Asset type: clean background plate for a 2.5D Johnny Castaway remaster. Edit the supplied image, preserving exact camera framing, centered enlarged island silhouette, palm position, warm campfire, sky, sunset, colors and all environment geometry. Remove the pixel-art man entirely, including fishing rod, fishing line, personal shadow and fishing equipment. Remove the raft entirely including mast/sail, ropes and its shadow. Reconstruct clean natural sand in those areas. Keep the campfire and its warm light, palm, subtle foliage, rocks and ocean intact. Do not add people, boats, objects, text, watermark or interface. Same wide landscape aspect ratio and composition. This is a background texture; there must be no character baked into it.
