# Audio original — validation du 18 octobre 2026

Les 23 ressources WAVE locales ont toutes un en-tête PCM mono 8 bits à
11 025 Hz. `audio_mixer.c` lit les échantillons float déjà décodés par
`resource.c` : aucune nouvelle banque de sons ni copie des samples.
`audio.c` utilise `sokol_audio.h` officiel, épinglé à la même révision que
les autres headers Sokol (`vendor/REVISION`), avec ALSA sous Linux.

## Déclenchements et durée de vie

L’observateur de présentation ajouté à `vm_core.c` transmet les IDs après
chaque pas réussi, même si `vm_advance` exécute plusieurs pas de rattrapage.
Le moteur, les scripts, le RNG et le canvas gardent leur comportement.
`engine.c`, `gfx.c`, `island.c`, `ttm.c` et `resource.c` sont inchangés.
Les sauts `--skip-steps` précèdent le branchement audio : pas de rafale de sons
historiques au lancement. Le mode headless ne crée aucun backend audio.

Le dernier ID reconnu interrompt immédiatement la voix précédente. Les IDs
absents sont ignorés. Le flag INI `Sounds` désactive la lecture. M bascule le
muet ; `--mute` et `--volume 0..100` ne sont pas persistés (gain initial 70 %).
Le muet laisse avancer les samples et ne remet pas les anciens sons en attente.

Le callback audio utilise un verrou court, sans allocation, accès disque ni
appel VM. Les tableaux décodés sont empruntés, puis libérés seulement après
`saudio_shutdown`, qui termine le thread. Une panne audio laisse fonctionner
la fenêtre. Rééchantillonnage linéaire avec phase conservée entre callbacks,
mono dupliqué en stéréo. Aucun remplacement HQ ni ambiance continue ajoutée.

## Résultats

- Build C99 avec `-Wall -Wextra -Wpedantic -Werror`.
- `make test` et `make sanitize` réussis ; ASan/UBSan avec `detect_leaks=0`.
- Nouveau test audio : interpolation, stéréo, interruption, bornes, fin de son,
  gain, muet, découpage des buffers. Deux VM comparées sur les 79 scènes :
  734 événements reçus dans le même ordre, aucune perte dans les rattrapages,
  canvas identique avec ou sans observateur ; flag INI transmis correctement.
- Suite graphique existante : 30 537 images reconstruites exactement.
- Flux headless 1 000 pas : `de8bfcfeb48c1c6b`, inchangé.
- Radeon 780M / RADV PHOENIX : scène de pêche 20, graine 4242, saut 100,
  horloge 1993-06-01T17:00. Quatre exécutions de 240 images, modes classique,
  moderne, muet et gain zéro : **aucune erreur Vulkan**.
- Sortie acceptée puis observée dans PipeWire : float32, 48 000 Hz, stéréo,
  buffer Sokol 1 024 frames. Capture ciblée du seul flux du projet, aucun micro
  ni son d’une autre application. Pic mesuré 0,22873 en classique et moderne ;
  pic et RMS exactement nuls pour muet et gain zéro. Cela confirme le transport
  audio ; aucune écoute humaine des haut-parleurs n’est revendiquée.
- Test natif complet Vulkan + audio sous ASan/UBSan : 240 images, deux sons
  reconnus, 195 016 samples de sortie non nuls ; aucune erreur détectée.
- Périphérique audio volontairement indisponible via un fichier ALSA isolé :
  avertissement attendu, sortie normale et 240 images rendues.
- Test shader GPU existant réussi : couleurs, damiers, œil et transparence
  identiques aux valeurs avant audio ; zéro erreur Vulkan.
- ELF lié dynamiquement : **239 384 octets** ; compact UPX : **88 888 octets**.
  Ces tailles excluent ressources et bibliothèques externes, dont `libasound`.

Journaux et mesures : `validation/audio-*`. Les anciens documents décrivant
un Johnny RGBA ou 64 Mio de staging sont des jalons historiques. Le rendu
actuel reste Johnny R8 exclusivement et décor moderne, sans changement shader.

## Essai

```sh
./preview-hybrid.sh --fullscreen --volume 70
```

M active/désactive le muet et Échap ferme la fenêtre. Pour compiler sous
Arch/CachyOS, `alsa-lib` fournit la dépendance ALSA ajoutée.

## Suivi : absence de son perçue dans l’aperçu

Le flux de la version release a été contrôlé : haut-parleurs internes,
application non muette à 100 %, `Sounds=1`, PCM non nul. Les autres applications
étaient audibles. L’utilisateur a confirmé entendre les trois bips du nouveau
`--audio-test`, puis le bruitage original en démarrant la pêche plus tôt.

Avec graine 4242 et horloge 1993-06-01T17:00, la pêche demande ID 4 au pas 82
et ID 16 au pas 88. Le saut de 100 pas les écartait volontairement avant le
branchement audio. Les deux événements suivants, ID 6 aux pas 134 et 161,
sont graves : 98,5 % de l’énergie de la ressource ID 6 se situe sous 400 Hz.
Ce point explique un aperçu difficile à entendre sur les petits haut-parleurs ;
il ne constitue pas une mesure de leur réponse acoustique.

Correction : saut ramené à **65 pas** dans le lanceur. La lecture des scènes
et les samples restent inchangés. Le test synthétique ne s’active qu’avec
`--audio-test` ; trois bips fondus aux secondes 1, 2 et 3, sans chargement VM
ou accès aux données originales. Build C99 strict et vérifications CLI réussis,
hash headless inchangé. ELF actuel : 239 416 octets ; UPX : 89 356 octets.
Les mesures initiales ci-dessus correspondent à la version avant ce diagnostic.
Journaux : `validation/audio-diagnosis-*`.
