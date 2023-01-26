PRACTICA SISTEMES OPERATIUS
Autors:
    Eric Planes Frias   -> eric.planes@studens.salle.url.edu

Valgrind utilitzat per validar nosaltres:
    valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --track-fds=yes --track-origins=yes

Grup: SO B #30

Notes importants:

    Hem utilitzat IPs home que ens interessaven i per fer la connexio hem permes la connexio per tots els ports, a la revisió de la Fase 3 vam demanar
    que si no eren correctes se'ns donguessin exemples. Donem per sentat que ja està bé fer-ho així ja que no se'ns va dir res sobre això.

    Per implementar l'error de tipus a les trames, hem fet que el client sigui el que talla sempre la connexio. És a dir:
        Si el servidro rep UNKNOWN, talla ell la connexió.
        Si el client rep UNKNOWN, envia al servidor UNKNOWN per a que talli ell la connexió.

    Sobre això tampoc se'ns va dir res malgrat informessim, pel que deduim que ja està bé així. Ho hem fet per controlar d'una sola banda les trames 
    desconegudes i hem cregut que és el servidor el que ho ha de gestionar.

    Hem solucionat tots els errors de les anteriors fases, ara els threads funcionen detached i realitzem cancels per tal de tancar-los.

Funcionament:
    Des de la propia carpeta SO_G30 podem fer un sol make per tal de compilar-ho tot. Aquest make executa els make propis de dins les carpetes. Els executables
    són: Fremen, Atreides i Harkonen. Es creen també a la carpeta SO_G30.

    Per tal d'executar Fremen i Atreides hem creat fremen.data i atreides.data, que són els fitxers de configuració d'aquests. Us hem deixat el valgrind
    que hem utilitzat nosaltres per validar que tot vagi bé i no hi hagi leaks de memòria.

    Hem afegit una llista d'usuaris emplenada per tal que pogueu comprovar que el search funciona amb varies trames. És users.data i es troba a Atreides.

    Veureu que durant l'execució es crearan diversos fitxers per tal de controlar els outputs, els hi hem donat permisos per tal que els pogueu veure (en el 
    moment de la creació).

    El fitxer scanner.sh és un bash que filtra els processos passats per parametre. Ho utilitzem per tal de tenir una llista dels pid que necessitem a Harkonen. 
    Ho hem fet per bash perquè era molt més ràpid i no hi ha res que digui que no es pot o que no s'hauria de fer. No ho hem trobat una mala decisió ja que si
    volguessim ampliar o modificar la comanda, es faria en un segon i fins i tot seria més ràpid que haver de modificar el codi, pel que és més mantenible.

Estructura:
    Com ja hem explicat a la memòria, les carpetes funcionen així:
        F_Atreides -> Conté Atreides.h i Atreides.c, a part de la carpeta d'informació. No ho hem dividit pels motius que ja hem explicat.
        F_Fremen -> Conté Fremen.h i Fremen.c, a part de la carpeta d'informació. No ho hem dividit pels motius que ja hem explicat.
        F_Harkonen -> Conté Harkonen.h i Harkonen.c.
        F_Extras -> Conté funcions genèriques o que utilitzaran tant Fremen com Atreides, com la d'emplenar la trama per tal de fer la comunicació. D'aquesta manera
        ens assegurem que tothom utilitzi el mateix i que no hi hagi errors ni diferències entre funcions. També conté els strings d'errors generics, de servidor i de
        client.
    
