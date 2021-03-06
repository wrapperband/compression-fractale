/* Ce fichier contient les paramètres de compression
 * Ces paramètes sont présent sous la forme de variables globales car ils sont quasi-constant,
 *  mais quand même modifiable au lancement du programme
 */

/* ****************** Gestion des ressources ****************** */

// Le nombre de processus qui sont crées pour la compression
int NB_THREADS = 10;

// Le nombre d'itérations effectuées pour décompresser une image
int ITERATIONS_DECOMPRESSION = 15;

/* ****************** Algorithme de compression ****************** */

// Limite d'acceptation pour les bouts lisses
float SEUIL_LISSAGE = 625;

// Limite d'acceptation pour les transfos en général
float SEUIL_VARIANCE = 625;

// Limite au dessus de laquelle on redécoupe la partie
float SEUIL_DECOUPE = 2000;

// Taille minimum de redécoupe
int TAILLE_MIN_DECOUPE = 4;

// Le nombre maximal de découpes qui pouront être faites, /!\ Prends le dessus sur TAILLE_MIN_DECOUPE
int NB_MAX_DECOUPE = 4;

/* ****************** Décompression ****************** */

int QUALITE_DECOMPRESSION = 5; // Ameiloration de la decompression, 1 : rien n'est changé

/* ****************** Débugage ****************** */

// Dossier contenant les resultats du debugage
const char* DOSSIER_DEBUG = "debug/";

// Si activé, désactive les affichages courrants en console
bool SILENCIEUX = false;

// Si activé, active les débugs en console
bool VERBOSE = true;
