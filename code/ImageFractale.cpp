#include "ImageFractale.h"

/* *************** Ouverture *************** */

ImageFractale::ImageFractale() {
	mLargeur = mHauteur = 0;
	mCouleur = mTransparence = false;
	mMoyenne = std::vector<unsigned char>();
	mIfs = std::vector<IFS>();
}

ImageFractale::ImageFractale(const char* fichier) : mMoyenne(std::vector<unsigned char>()), mIfs(std::vector<IFS>()) {
	std::ifstream f(fichier, std::ios::in | std::ios::binary);

	Pack_Entete entete;
	f.read((char*)&entete, sizeof(Pack_Entete));
		mLargeur = entete.largeur;
		mHauteur = entete.hauteur;
		mCouleur = entete.couleur;
		mTransparence = entete.transparence;

	int nbCouches = mCouleur ? 3 : 1; // Calcul du nombre de couches
	if(mTransparence) nbCouches += 1;

	for(int i=0 ; i<nbCouches ; i++) {
		Pack_IFS ifs;
		f.read((char*)&ifs, sizeof(Pack_IFS));
		mIfs.push_back( unpack_IFS(ifs) );
		mMoyenne.push_back( unpack_moyenne(ifs) );

		int nbBlocs = std::ceil((float)mLargeur/ifs.decoupePetit) * std::ceil((float)mHauteur/ifs.decoupePetit);
		lireCorrespondancesFichier(f, nbBlocs, mIfs[i].correspondances);
	}

	f.close();
}

ImageFractale ImageFractale::compresser(const char* fichier, int precisionPetit, int precisionGros, bool couleur, bool transparence) {
    /* Retourne la compression d'un fichier au format ImageFractale
     * Entrées :
     *  - fichier : l'adresse du fichier (.png) à lire
     *  - precisionPetit : la taille des petits blocs
     *     | Réduire cette grandeur augmente la qualité
     *     | Réduire cette grandeur augmentera la taille du fichier final
     *  - precisionGros : la taille des gros blocs
     *     | /!\ Doit être plus grand que 'taillePetit' (recommandation : 150%)
     *     | Réduire cette grandeur augmente la qualitéen général (cf. recommandation)
     *     | Réduire cette grandeur augmente la durée de calculs
     *  - couleur : true si l'image est en couleur (x3 temps de calculs)
     *  - transparence : true s'il y a une couche alpha
     */
	ImageMatricielle image(fichier, 0);
	ImageFractale retour;
		retour.mLargeur = image.getLargeur();
		retour.mHauteur = image.getHauteur();
		retour.mCouleur = couleur;
		retour.mTransparence = transparence;

	COUT << "Dimension de l'image : " << retour.mLargeur << "x" << retour.mHauteur << std::endl;

	if(couleur) {
		const char* message[] = {" - Couche rouge ", " - Couche verte ", " - Couche bleue "};
		for(int i=0 ; i<3 ; i++) {
			ImageMatricielle imageTr(fichier, i);
			retour.mIfs.push_back( imageTr.chercherIFS(precisionPetit, precisionGros, message[i]) );
			retour.mMoyenne.push_back( imageTr.moyenne() );
		}
	}
	else {
		retour.mIfs.push_back( image.chercherIFS(precisionPetit, precisionGros, " - Couche NVDG  ") );
		retour.mMoyenne.push_back( image.moyenne() );
	}
	if(transparence) {
		ImageMatricielle imageTr(fichier, 3);
		retour.mIfs.push_back( imageTr.chercherIFS(precisionPetit, precisionGros, " - Couche alpha") );
		retour.mMoyenne.push_back( imageTr.moyenne() );
	}

	return retour;
}

/* *************** Enregistrement *************** */

void ImageFractale::enregistrer(const char* fichier) const {
	std::ofstream f(fichier, std::ios::trunc | std::ios::binary);
	if (!f.is_open()) std::cout << "Impossible d'ouvrir le fichier '" << fichier << "'" << std::endl;

	Pack_Entete entete = packer_entete(*this); // Récupère les données binaires de l'en-tête
	f.write((char*)&entete, sizeof(Pack_Entete));

	for(int i=0 ; i < mIfs.size() ; i++) {
		Pack_IFS ifs = packer_ifs(mIfs[i], mMoyenne[i]);
		f.write((char*)&ifs, sizeof(Pack_IFS));

		const std::vector<Correspondance> &mCorrespondances = mIfs[i].correspondances;
		std::cout << mCorrespondances.size() << std::endl;
		for(int j=0 ; j < mCorrespondances.size() ; j++) {
			Pack_Correspondance correspondance = packer_correspondance(mCorrespondances[j]);
			f.write((char*)&correspondance, SIZEOF_PACK_CORRESPONDANCE);
		}
	}

	f.close();
}

void ImageFractale::exporter(const char* fichier) {
    /* Crée un rendu de l'image et l'exporte au format (.png)
	*/
	extern int ITERATIONS_DECOMPRESSION, QUALITE_DECOMPRESSION;

	grandir(QUALITE_DECOMPRESSION);

	std::vector<ImageMatricielle*> couche(mIfs.size(), NULL);
	for(int i=0 ; i < mIfs.size() ; i++) {
		couche[i] = new ImageMatricielle(mLargeur, mHauteur);
		couche[i]->remplir(mMoyenne[i]);
		for(int k=0 ; k < ITERATIONS_DECOMPRESSION ; k++) {
			ImageMatricielle *nouveau =  new ImageMatricielle(couche[i]->appliquerIFS(mIfs[i]));
			delete couche[i]; // On désaloue pour ne pas créer de fuite de mémoire
			couche[i] = nouveau;
		}
		couche[i]->lisser(QUALITE_DECOMPRESSION-1);
		couche[i]->retrecir(QUALITE_DECOMPRESSION);
	}

	retrecir(QUALITE_DECOMPRESSION);

	std::vector<unsigned char> pixels;
	for(int j=0 ; j<mHauteur ; j++) {
		for(int i=0 ; i<mLargeur ; i++) {
			if(mCouleur) {
				for(int n=0 ; n<3 ; n++) // Ajoute les 3 couleurs
					pixels.push_back( (unsigned char)( (*couche[n])[i][j] ) );
			}
			else{
				for(int n=0 ; n<3 ; n++) // Pose 3 fois la même couleur
					pixels.push_back( (unsigned char)( (*couche[0])[i][j] ) );
			}
			if(mTransparence) pixels.push_back( (unsigned char)( (*couche[couche.size()-1])[i][j] ) );
			else pixels.push_back(255);
		}
	}

	std::vector<unsigned char> png;
	unsigned error = lodepng::encode(png, pixels, mLargeur, mHauteur);
	if(!error) lodepng::save_file(png, fichier);

	if(error) std::cout << fichier << " -> png encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
}

void ImageFractale::debugSplit() const {
	extern const char* DOSSIER_DEBUG;
	for( int i = 0 ; i < mIfs.size() ; i++ ) {
		ImageMatricielle img(mLargeur, mHauteur);
		std::vector<ImagePart> parties = img.decouper( mIfs[i].decoupePetit );
		parties = ImageMatricielle::adapterDecoupe( parties, mIfs[i].correspondances );
		for( int j = 0 ; j < parties.size() ; j++ ) {
			parties[j].encadrer();
		}

		std::stringstream nom;
		nom << DOSSIER_DEBUG << "grille-" << i << ".png";
		img.sauvegarder( nom.str().c_str() );
	}
}

void ImageFractale::grandir(int grandissement) {
	mHauteur *= grandissement;
	mLargeur *= grandissement;
	for(int i=0 ; i < mIfs.size() ; i++) {
		mIfs[i].decoupeGros *= grandissement;
		mIfs[i].decoupePetit *= grandissement;
	}
}

void ImageFractale::retrecir(int reduction) {
	mHauteur /= reduction;
	mLargeur /= reduction;
	for(int i=0 ; i < mIfs.size() ; i++) {
		mIfs[i].decoupeGros /= reduction;
		mIfs[i].decoupePetit /= reduction;
	}
}

/* *************** Getters / Setters *************** */

int ImageFractale::getLargeur() const { return mLargeur; }
int ImageFractale::getHauteur() const { return mHauteur; }
bool ImageFractale::isCouleur() const { return mCouleur; }
bool ImageFractale::isTransparent() const { return mTransparence; }
