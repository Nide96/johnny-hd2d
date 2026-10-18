#!/usr/bin/env python3
"""Describe measured sprite substitution coverage; no original pixels exported."""
from pathlib import Path
from collections import Counter
import json
root=Path(__file__).resolve().parent.parent
scenes=json.loads((root/'assets/scenes/inventory.json').read_text())['scenes']
active=json.loads((root/'assets/characters/active-retro.json').read_text())
covered=set()
for sheet in active['sheets']:
    data=json.loads((root/'assets/characters'/sheet['metadata']).read_text())
    covered.update((sheet['resource'],c['sprite']) for c in data['cells'] if c.get('enabled',True))
bitmap_count=len(covered)
covered.update({("LITEBULB.BMP",1),("LITEBULB.BMP",4)})
rows=[];remaining=Counter();affected=0
for s in scenes:
    hd=sum(a['draws'] for a in s['sprites'] if (a['resource'],a['id']) in covered)
    affected+=bool(hd)
    families=sorted({a['resource'] for a in s['sprites'] if (a['resource'],a['id']) in covered})
    for a in s['sprites']:
        if (a['resource'],a['id']) not in covered and a['resource']!='TRUNK.BMP':remaining[a['resource']]+=a['draws']
    rows.append(f"| {s['scene']} | {s['steps']} | {s['modern_allowed_steps']} | {s['compatible_steps']} | {hd} | {', '.join(families) or '—'} |")
text=f'''# Couverture des scènes — personnage retro v1

Inventaire reproductible : graine 4242, horloge 1993-06-01T10:00, chapitre 1.
Chaque scène est forcée puis exécutée jusqu’à sa fin, sans boucle de démo.
Les tirages aléatoires d’une autre graine peuvent emprunter d’autres branches.
Les comptes sont des commandes de dessin par pas VM, pas des images GPU.

**Archive de correspondances seulement : le renderer actuel n’active aucun personnage RGBA.**

**{bitmap_count} dessins bitmap HD et 2 symboles vectoriels ; {affected}/79 scènes rencontrent au moins une
dessin remplaçable dans ce parcours.** Cela ne signifie pas que ces scènes sont
entièrement redessinées. Les accessoires et corps partiels sans correspondance restent originaux.
Les colonnes de garde héritée et de compatibilité sont les métadonnées VM mesurées,
pas la sélection du renderer hybride. Celui-ci harmonise aussi les compositions
complètes non décomposables ; voir `VALIDATION-UNIFORME.md`. Les données ne prouvent
pas une couverture GPU exhaustive.

Régénération depuis le dossier projet : `make inventory`, puis
`python tools/report_scene_coverage.py`. Les originaux doivent être présents.

| Scène | Pas VM | Garde VM héritée | Compatibles à ce pas | Dessins HD candidats | Familles enrichies |
|---:|---:|---:|---:|---:|---|
'''+ '\n'.join(rows)+'''

## Ressources non remplacées les plus dessinées

Liste brute incluant personnages, fragments, animaux et accessoires : ne pas
transformer automatiquement tous ces dessins en corps entiers de Johnny.

'''+ '\n'.join(f'- {name} : {count} dessins' for name,count in remaining.most_common(15))+'\n'
(root/'assets/scenes/COVERAGE.md').write_text(text)
print(f'{bitmap_count} bitmap HD assets + 2 vector symbols; candidates in {affected}/79 scene runs')
