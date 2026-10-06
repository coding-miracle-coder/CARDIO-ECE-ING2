# POUR MAXIME

Avant de commencer à coder :

```bash
git pull --ff-only

Ensuite tu codes.
Avant de push :
pio run

Si ça ne compile pas :
NE PUSH PAS.
Si ça compile :
git add -A
git commit -m "Décris clairement ce que tu as modifié"
git push

Règles importantes
- Ne pas utiliser "Upload files" sur GitHub pour modifier le projet.
- Ne pas créer de fichiers/libs uniquement en local sans les ajouter au repo.
- Toute librairie PlatformIO doit être ajoutée dans platformio.ini.
- Si tu ajoutes un #include, le fichier doit exister dans le repo ou provenir d'une dépendance déclarée.
- Ne pas renommer .hpp en .h.
- Si git pull provoque un conflit : ne rien push avant résolution.
- Si tu ne sais pas quoi faire : demande avant de toucher au Git.
Test final
Après un pull, ceci doit fonctionner :
pio run

sur n'importe quel PC ayant cloné le repo.
