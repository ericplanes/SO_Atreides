#!/bin/bash

#Agafem els 7 primers digits de l'usuari
USER=$(whoami | cut -c1-7)

#Filtrem ps -u primer per usuari i després per proces
ps -u | grep ${USER} | grep $1 | awk '{print $2}'