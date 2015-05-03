#/bin/sh
dot -Tpng rules.dot > rules.png
dot -Tpng proof.dot > proof.png
gnome-open rules.png &
gnome-open proof.png &
