#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10// Le nombre d'enregistrements maximal qu'un bloc peut contenir/

/******************************************************************************************************************************
 |                                              PARTIE 1 : méthode TOF                                                        |
 *****************************************************************************************************************************/

/***********************************************************
 |       Déclaration de la structure de fichier TOF        |
 ***********************************************************/
/** Structure d'enregistrement **/
typedef struct
{
    long long cle; // numéro de la clé
    int efface; // le champ effacé pour la suppression logique

} Tenreg_TOF;

/** Structure de bloc **/
typedef struct
{
    Tenreg_TOF tab[MAX]; //tableau d'enregistrements
    int nb; // nombre d'enregistrements dans le bloc
} Tbloc_TOF;

/** Structure de l'entête du fichier **/
typedef struct
{
    int nbBloc; // Nombre de blocs dans le fichier
    int nbEnreg; // Nombre d'enregistrements dans le fichier
} Entete_TOF;

/** Structure du fichier LOF **/
typedef struct
{
    FILE *fichier;
    Entete_TOF entete;
} TOF;


/*******************************************************
 |       Implémentation du modèle de fichier TOF       |
 ******************************************************/

/** Fonction d'ouverture ou création d'un fichier TOF **/

TOF* ouvrir_TOF(char *nomf, char mode)
{
    TOF* f = malloc(sizeof(TOF)); // Allouer dynamiquement une zone mémoire pour le fichier
    //mode ancien
    if((mode == 'A') || (mode == 'a'))
    {
        f->fichier = fopen(nomf, "rb+"); // ouvrir un fichier existant en mode binaire lecture/écriture
        if(f->fichier == NULL)
            return NULL;
        else
            fread(&(f->entete), sizeof(Entete_TOF), 1, f->fichier); // chargement de l'entête dans la structure
    }
    //mode nouveau
    else if((mode == 'N') || (mode == 'n'))
    {
        f->fichier = fopen(nomf, "wb+"); // ouvrir un nouveau fichier en mode binaire lecture/écriture
        if(f->fichier == NULL)
            return NULL;
        else
        {
            // initialisation de l'entête dans la structure
            f->entete.nbBloc = 0;
            f->entete.nbEnreg = 0;
        }
    }
    else return NULL;
    return f;
}

/*------------------------------------------------------------------------------*/
/** Procédure de fermeture d'un fichier TOF **/

void fermer_TOF(TOF *f)
{
    rewind(f->fichier); // positioner le curseur au début du fichier
    fwrite(&(f->entete), sizeof(Entete_TOF), 1, f->fichier); // sauvegarde l'entête en début du fichier
    fclose(f->fichier); // ferme le fichier
    free(f); // libère l'espace mémoire occupé par la structure
}

/*------------------------------------------------------------------------------*/
/** Procédure de lecture d'un bloc methode TOF **/

void lireDir_TOF(TOF *f, int Num_Bloc, Tbloc_TOF *buf)//lecture directe du contenu de fichier à la position i ds le buf
{
    if(Num_Bloc <= (f->entete).nbBloc)
    {
        fseek(f->fichier, sizeof(Entete_TOF) + (Num_Bloc-1) * sizeof(Tbloc_TOF), SEEK_SET);//se positionner à la place exacte de lecture
        fread(buf, 1, sizeof(Tbloc_TOF), f->fichier);//lecture
    }
}

/*------------------------------------------------------------------------------*/
/** Procédure d'écriture d'un bloc méthode TOF **/

void ecrireDir_TOF(TOF *f, int Num_Bloc, Tbloc_TOF *buf)
{
    if(Num_Bloc <= (f->entete).nbBloc)
    {
        fseek(f->fichier, sizeof(Entete_TOF) + (Num_Bloc-1) * sizeof(Tbloc_TOF), SEEK_SET);//se positionner à la place exacte d'ecriture
        fwrite(buf, 1, sizeof(Tbloc_TOF), f->fichier);//écriture
    }
}

/*------------------------------------------------------------------------------*/
/** Fonction de lecture de l'entête méthode TOF **/

/* lorsque i=1 ====> lecture du nombre de blocs
   lorsque i=2 ====> lecture du nombre d'enregistrements */
int entete_TOF(TOF *f,int i)//retourner la cractéristique i
{
    if(i == 1) return (f->entete).nbBloc;
    else if(i == 2) return (f->entete).nbEnreg;
    else return -1;
}

/*------------------------------------------------------------------------------*/
/** Procédure de modification de l'entête méthode TOF **/

/* lorsque i=1 ====> modification du nombre de blocs
   lorsque i=2 ====> modification du nombre d'enregistrement */
void aff_entete_TOF(TOF *f, int i, int val)
{
    if(i == 1) (f->entete).nbBloc = val;
    else if(i == 2) (f->entete).nbEnreg = val;
}

/*------------------------------------------------------------------------------*/
/** allocation d'un nouveau bloc méthode TOF **/

int alloc_bloc_TOF(TOF *f,Tbloc_TOF *buf)
{
//initialise un buffer
    (*buf).nb = 0;
    for(int i=0; i<MAX; i++) (*buf).tab[i].efface = 1;
    //incrémenter le numéro du dernier bloc dans l'en-tête
    aff_entete_TOF(f,1,entete_TOF(f,1)+1);
    return entete_TOF(f,1);//retourner le numéro du bloc alloué
}
/*******************************************************************************
 |      Modules nécessaires pour la manipulation des fichier TOF     |
 ******************************************************************************/
/*------------------------------------------------------------------------------*/
/*** module de recherche dans le fichier TOF ***/

void rechDicho_TOF(long long CLE, TOF* f, int* trouv,int* efface,int* i,int* j,int *nb_lireDir, int *nb_ecrireDir)
{
    Tbloc_TOF buf;
    *trouv = 0;
    *efface = 0;
    int stop = 0;
    int infB, supB; // les numéros des blocs sup et inf de la recherche externe
    int infE, supE; // les positions des enregistrements sup et inf de la recherche interne
    /* Traitement du cas ou le fichier est vide */
    if (entete_TOF(f,1) == 0)
    {
        *i = 1;
        *j = 0;
        return;
    }
    /* Recherche externe */
    infB = 1; // le premier bloc
    supB = entete_TOF(f, 1); //le dernier bloc
    while(infB<=supB && !(*trouv) && !stop)
    {
        *i = (infB + supB) / 2; // le numéro du bloc du milieu
        lireDir_TOF(f, *i, &buf);
        (*nb_lireDir)++;
        if (buf.tab[0].cle <= CLE && CLE <= buf.tab[buf.nb-1].cle)
        {
            /* Recherche interne */
            infE = 0; // le premier enregistrement du bloc
            supE = buf.nb - 1; // le dernier enregistrement du bloc
            while(infE<=supE && !(*trouv))
            {
                *j = (infE + supE) / 2; // l'indice de l'enregistrement du milieu
                if (buf.tab[*j].cle == CLE)
                {
                    *trouv = 1;
                    if (buf.tab[*j].efface==1) *efface=1;

                }
                else
                {
                    if (buf.tab[*j].cle > CLE)
                        supE = *j - 1;
                    if (buf.tab[*j].cle < CLE)
                        infE = *j + 1;
                }
            }
            if (infE>supE) // le numéro recherché n'existe pas dans le bloc
                *j = infE; // la position ou il faut l'inséerer alors
            stop = 1;
            /* Fin de la recherche interne */
        }
        else
        {
            if (buf.tab[0].cle > CLE)
                supB = *i - 1;
            if (CLE > buf.tab[buf.nb-1].cle)
                infB = *i + 1;
        }
    }
    if (infB>supB && CLE<buf.tab[0].cle) // la clé recherchée est plus petite que
        *j = 0;                                 // tous les clés qui existent dans le fichier

    if (infB>supB && CLE>buf.tab[buf.nb-1].cle) // la clé recherchée est plus grande que
    {
        // tous les clés qui existent dans le fichier

        if (buf.nb == MAX) // le dernier bloc est plein
        {
            *i = infB; // il faut insérer l'enregistrement dans un nouveau bloc
            *j = 0;
        }
        else // le dernier bloc n'est pas encore plein
            *j = buf.nb; // il faut l'insérer dans le dernier bloc à la première position libre
    }
    /* Fin de la recherche externe */
}

/*------------------------------------------------------------------------------*/
/** module d'insertion dans le fichier TOF **/

int insertion_TOF (TOF* f, Tenreg_TOF enreg, int *nb_lireDir, int *nb_ecrireDir)
{
    enreg.efface=0;
    int trouv,i,j;
    int efface =0;
    // On commence par rechercher la clé pour localiser l'emplacement (i,j)
    // où doit être insérer l'enregistrement s'il n'existe pas encore le numéro dans le fichier
    rechDicho_TOF(enreg.cle,f,&trouv,&efface,&i,&j,nb_lireDir,nb_ecrireDir);

    if (trouv)
    {
        if(efface==0) //la clé est trouvée
            return 0;
        else
        {
            // la clé est trouvé mais elle est supprimer grâce au champ logique
            Tbloc_TOF buf;
            lireDir_TOF(f,i,&buf);
            (*nb_lireDir)++;
            buf.tab[j].efface=0;//repositioner le champ éffacé à faux
            buf.nb++;
            ecrireDir_TOF(f,i,&buf);
            (*nb_ecrireDir)++;
            aff_entete_TOF(f,2,entete_TOF(f,2)+1); /* incrémenter le champ de l'entête qui contient
                                          le nombre d'enregistrements */

            return 1;

        }
    }
    else // la clé n'existe pas dans le fichier, il faut l'insérer à l'emplacement (i,j)
    {
        Tbloc_TOF buf;
        Tenreg_TOF x;
        int k;
        int continu=1;
        while ((continu==1) && (i<=entete_TOF(f,1)))
        {
            lireDir_TOF(f,i,&buf);
            (*nb_lireDir)++;
            x = buf.tab[buf.nb-1]; // sauvegarder le dernier enregistrement du bloc

            /* Décalages intra-bloc */
            for(k = buf.nb-1; k>j; k--)
                buf.tab[k] = buf.tab[k-1];
            buf.tab[j] = enreg;
            if(x.efface==0)
            {
                if(buf.nb < MAX) // si le bloc n'est pas plein, on remet x dans permier indice libre du bloc
                {
                    buf.nb++;
                    if (j == buf.nb-1) // le nouvel enregistrement est le dernier enregistrement du fichier
                        buf.tab[buf.nb-2] = x;
                    else // le nouvel enregistrement a été inséré entre deux enregistrements
                        buf.tab[buf.nb-1] = x;
                    ecrireDir_TOF(f,i,&buf);
                    (*nb_ecrireDir)++;
                    continu = 0;
                }
                else // si le bloc est plein, x doit être inséré dans le bloc suivant i+1 à la position 0
                {
                    ecrireDir_TOF(f,i,&buf);
                    (*nb_ecrireDir)++;
                    i++;
                    j = 0;
                    enreg = x; // l'enregistrement qui doit être inséré à l'emplacement (i,0)
                }
            }
            else continu=0;
        }
        // si on dépasse la fin de fichier, on rajoute un nouveau bloc contenant un seul enregistrement
        if (i>entete_TOF(f,1))
        {
            i = alloc_bloc_TOF(f,&buf);
            buf.tab[0] = enreg;
            buf.nb = 1;
            ecrireDir_TOF(f,i,&buf);
            (*nb_ecrireDir)++;
        }
        aff_entete_TOF(f,2,entete_TOF(f,2)+1); /* incrémenter le champ de l'entête qui contient
                                          le nombre d'enregistrements */
        return 1;
    }
}

/***    Suppression TOF     ****/

int suppression_TOF(TOF* f, Tenreg_TOF enreg, int *nb_lireDir, int *nb_ecrireDir)
{
    int trouv;
    int efface;
    int i,j;
    Tbloc_TOF buf;
// on commence par rechercher la clé c pour localiser l'emplacement (i,j) de l'enreg à supprimer
    rechDicho_TOF(enreg.cle,f,&trouv,&efface,&i,&j,nb_lireDir,nb_ecrireDir);
// ensuite on supprime logiquement l'enregistrement
    if ( trouv==1 )
    {
        lireDir_TOF(f,i,&buf);
        nb_lireDir++;
        buf.tab[j].efface=1;
        buf.nb--;
        ecrireDir_TOF( f, i, &buf );
        nb_ecrireDir++;
        aff_entete_TOF(f,2,entete_TOF(f,2)-1); /* décrémenter le champ de l'entête qui contient
                                          le nombre d'enregistrements */

    }
}

/***    Affichage TOF   ****/

void affichage_TOF(TOF *f)
{
    int i,j;
    Tbloc_TOF buff;
    printf("\n Affichage du Fichier TOF:\n");
    printf("Entete TOF: nbbloc=%d nbenr=%d\n",entete_TOF(f,1),entete_TOF(f,2));
    for (i=1; i<=entete_TOF(f,1); i++)
    {
        lireDir_TOF(f,i,&buff);
        printf(" <nb=%d >: ",buff.nb);

        printf(" [ ");
        for (j=0; j<MAX; j++)
        {
            if (buff.tab[j].efface==0)
                printf(" %lld ",buff.tab[j].cle);
        }
        printf(" ]\n");
    }
}

/******************************************************************************************************************************
 |                                              PARTIE 2 : Méthode LOF                                                        |
 *****************************************************************************************************************************/
/***********************************************************
 |       Déclaration de la structure de fichier LOF        |
 ***********************************************************/
/*********structure Tenreg ***********/
typedef struct
{
    long long cle;//la clé
    int efface;//le champs effacé
} Tenreg_LOF; //la declaration de l'enregistrement


/**************structure de Tbloc*******************/
typedef struct
{
    Tenreg_LOF t[MAX];//tableau d'enregistrement
    int nb;//le nombre d'enregistrement dans le bloc (dans le tableau)
    int suiv;//le n° du prochain bloc
} Tbloc_LOF; //declaration du bloc

/**************structure d'en_tete***************/
typedef struct
{
    int num_Btete;//le n° du bloc tete(premier)
    int nb_enreg;//le nombre d'enregistrements dans le fichier
    int nb_bloc;//le numero du dernier bloc
} entete_LOF; //declaration de l'en-tête

/************structure de fichierLOF**************/
typedef struct
{
    FILE *f;//declaration du fichier
    entete_LOF entete;//declaration de l'en-tête
} LOF; //declaration de la structure LOF



/*******************************************************
 |       Implémentation du modèle de fichier LOF       |
 ******************************************************/

/** Fonction d'ouverture ou création d'un fichier LOF **/
LOF* ouvrir_LOF (char *nomf, char mode)//ouvrir un fichier de type LOF en mode voulu
{
    LOF* fichier = malloc(sizeof(LOF));//allouer dynamiquement une zone en mémoire centrale pour le fichier
    if ((mode=='n')||(mode=='N'))//si le mode est nouveau (le fichier n'existe pas déjà) alors
    {
        fichier->f=fopen(nomf,"wb+");//ouvrir un fichier en mode ecriture binaire
        fichier->entete.num_Btete=-1;//initialiser les champs de l'en_tete
        fichier->entete.nb_enreg=0;//initialiser le nombre d'enregistrements à 0
        fichier->entete.nb_bloc=0;//initialiser le nombre de blocs à 0
        fwrite(&(fichier->entete),sizeof(entete_LOF),1,fichier->f);//ecrire l'en-tête dans le fichier binaire
    }
    else
    {
        if ((mode=='a')||(mode=='A'))//si le mode est ancien (le fichier existe)
        {
            fichier->f=fopen(nomf,"rb+");//ouvrir le fichier en mode lecture binaire
            if (fichier->f==NULL)
                return NULL;//si erreur d'ouverture
            else//sinon
                fread(&(fichier->entete),sizeof(entete_LOF),1,fichier->f);//récuperer le contenu de l'en-tête dans la variable entete
        }
        else return NULL;
    }
    return fichier;//retourner le pointeur du fichier
}

/************       FERMITURE LOF      ***************/
void fermer_LOF (LOF *fichier)//fermer un fichier
{
    rewind(fichier->f);//on se positionne au debut de fichier
    fwrite(&(fichier->entete),sizeof(entete_LOF),1,fichier->f);//on enregistre les modifications effectuées sur l'en_tete
    fclose(fichier->f);//on ferme le fichier
    free(fichier);//liberer la zone LOF reservée
}

/****************       LECTURE DIRECTE LOF     *************/
void lireDir_LOF (LOF *fichier,int i,Tbloc_LOF *buf)//lecture directe du contenu de fichier à la position i dans le buf
{
    fseek(fichier->f,sizeof(entete_LOF)+(i)*sizeof(Tbloc_LOF),SEEK_SET);//se positionner à la place exacte
    fread(buf,sizeof(Tbloc_LOF),1,fichier->f);//lecture
}

/***************    ECRITURE DIRECTE LOF   ***********/
void ecrireDir_LOF (LOF *fichier,int i,Tbloc_LOF *buf)//ecriture directe du contenu de buf ds le fichier à la position i
{
    fseek(fichier->f,sizeof(entete_LOF)+(i)*sizeof(Tbloc_LOF),SEEK_SET);//se positionner à la place exacte
    fwrite(buf,sizeof(Tbloc_LOF),1,fichier->f);//ecriture
}


/*******************    Entete_LOF  *********************/
int Entete_LOF(LOF *fichier,int num_caract )//retourner la cracterstique num_caract
{
    if(num_caract==1)
        return fichier->entete.num_Btete;
    else if (num_caract==2)
        return fichier->entete.nb_enreg;
    else if (num_caract==3)
        return fichier->entete.nb_bloc;
    else return -1;

}


/****************AFF_ENTETE*******************/
void aff_entete_LOF (LOF *fichier,int num_caract,int val)//affecter la valeur val à la caracteristique num_caract
{
    if(num_caract==1)
        fichier->entete.num_Btete=val;
    else if (num_caract==2)
        fichier->entete.nb_enreg=val;
    else if (num_caract==3)
        fichier->entete.nb_bloc=val;
}
/*******************alloc_bloc***********************/
int alloc_bloc_LOF(LOF *fichier,Tbloc_LOF* buf) //Aloue un bloc et retourne son numéro
{
//initialise le buffer
    for(int i=0; i<MAX; i++)
    {
        buf->t[i].cle = 0;
        buf->t[i].efface = 1;
    }
    buf->nb=0;
    buf->suiv=-1;

    aff_entete_LOF(fichier,3,Entete_LOF(fichier,3)+1);//recuperer le num de dernier bloc et l'incrémenter
    return Entete_LOF(fichier,3);//retourne le num du nouveau bloc
}

/*******************************************************************************
 |      Modules nécessaires pour la manipulation des fichier LOF     |
 |                   Avec quelques améliorations                     |
 ******************************************************************************/


/*****************recherche LOF *******************/

void recherche_LOF(LOF* f,long long CLE, int* trouv,int* efface,int* i,int* num_blocp, int* j,int *nb_lireDir, int *nb_ecrireDir)
{
// La recherche externe est séquentielle et la recherche interne est dichotomique
    Tbloc_LOF buf;
    *i = -1;
    *j = 0;
    *trouv = 0;
    *efface = 0;
    int stop = 0;
    int inf, sup; // les positions des enregistrements sup et inf de la recherche interne
    /* Traitement du cas ou le fichier est vide */
    if (Entete_LOF(f,1) == -1)
    {
        *i = -1;
        *j = 0;
        return;
    }
    /* Recherche externe */
    *i = Entete_LOF(f,1); // le premier bloc
    *num_blocp=*i;
    while(*i!=-1 && !(*trouv) && !stop)
    {
        lireDir_LOF(f, *i, &buf);
        (*nb_lireDir)++;
        if (CLE >= buf.t[0].cle && CLE <= buf.t[buf.nb-1].cle)
        {
            /* Recherche interne */
            inf = 0; // le premier enregistrement du bloc
            sup = buf.nb - 1; // le dernier enregistrement du bloc
            while(inf<=sup && !(*trouv))
            {
                *j = (inf + sup) / 2; // l'indice de l'enregistrement du milieu
                if (buf.t[*j].cle == CLE)
                {
                    *trouv = 1;
                    if (buf.t[*j].efface==1) *efface=1;
                    stop=1;

                }
                else
                {
                    if (CLE < buf.t[*j].cle)
                        sup = *j - 1;
                    else if (CLE > buf.t[*j].cle)
                        inf = *j + 1;
                }
            }
            if (inf>sup)  // le numéro recherché n'existe pas dans le bloc
                *j = inf; // la position ou il faut l'inséerer alors
            stop = 1;

            /* Fin de la recherche interne */


        }
        else
        {
            if (CLE > buf.t[buf.nb-1].cle)
            {
                //avancer au prochain bloc
                *num_blocp = *i;
                *i=buf.suiv;
                *j=0;
            }
            if (CLE < buf.t[0].cle)
            {
                *j=0;
                stop=1;
            }
        }

    }
}

/**----------------Procédure d'insertion 1 dans le fichier LOF------------------**/

void insertion1_LOF(LOF *fichier,Tenreg_LOF enreg,int *nb_lect_limite, int *nb_lect, int *nb_ecr)
{
    enreg.efface=0;
    int i=-1,j=0,trouv=0,efface=0,k=0,n=0,num_bloc_prec=-1;
    Tbloc_LOF buf,buf2,buf3;
    Tenreg_LOF x;
    if (fichier->f!=NULL)
    {
        recherche_LOF(fichier,enreg.cle,&trouv,&efface,&i,&num_bloc_prec,&j,nb_lect,nb_ecr);
        //printf("\n > i=%d , j= %d \n",i,j);
        if(trouv && !efface)//la clé existe et est non supprimé
            return;
        else
        {
            if (trouv && efface)
            {
                //la clé existe et est supprimée
                lireDir_LOF(fichier,i,&buf);
                (*nb_lect)++;
                buf.t[j].efface=0;//repositioné le champ efface à faux
                buf.nb++;
                ecrireDir_LOF(fichier,i,&buf);
                (*nb_ecr)++;
                aff_entete_LOF(fichier,2,Entete_LOF(fichier,2)+1);
                return;

            }
            else
            {
                // la clé n'existe pas dans le fichier, il faut l'insérer à l'emplacement (i,j)

                if((i==-1)&&(Entete_LOF(fichier,1)==-1))//insertion dans un fichier vide
                {
                    i = alloc_bloc_LOF(fichier,&buf);
                    buf.t[0] = enreg;
                    buf.nb=1;
                    ecrireDir_LOF(fichier,i,&buf);
                    (*nb_ecr)++;
                    aff_entete_LOF(fichier,1,i);

                }
                else if((i==-1)&&(Entete_LOF(fichier,1)!=-1))//insertion en fin de fichier
                {
                    //allocation d'un nouveau bloc
                    i = alloc_bloc_LOF(fichier,&buf2);
                    lireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_lect)++;

                    buf.suiv=i;
                    ecrireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_ecr)++;
                    buf2.t[0] = enreg;
                    buf2.nb=1;
                    ecrireDir_LOF(fichier,i,&buf2);
                    (*nb_ecr)++;
                }

                else//insertion dans un bloc (au milieu du fichier)
                {
                    int continu=1;
                    int cpt_lect=0;
                    while ((continu==1) && (cpt_lect<*nb_lect_limite) && (i!= -1) )
                    {
                        lireDir_LOF(fichier,i,&buf);
                        (*nb_lect)++;
                        cpt_lect++;
                        x = buf.t[buf.nb-1]; // sauvegarder le dernier enregistrement du bloc

                        /** Décalages intra-bloc **/
                        for(int k = buf.nb-1; k>j; k--)
                            buf.t[k] = buf.t[k-1];
                        /**/
                        buf.t[j] = enreg;

                        if(x.efface==0)
                        {
                            if(buf.nb < MAX) // si le bloc n'est pas plein, on remet x dans la fin du bloc
                            {
                                buf.nb++;
                                buf.t[buf.nb-1] = x;
                                ecrireDir_LOF(fichier,i,&buf);
                                (*nb_ecr)++;
                                continu = 0;

                            }
                            else // si le bloc est plein, x doit être inséré dans le bloc i+1 à la position 0
                            {
                                /**Décalage inter-blocs**/
                                ecrireDir_LOF(fichier,i,&buf);
                                (*nb_ecr)++;
                                i=buf.suiv;
                                j = 0;
                                enreg = x; // l'enregistrement qui doit être inséré à l'emplacement (i,0)
                            }/**/

                        }
                        else continu=0; //si l'enregistrement est éffacé on le n'insère pas
                    }
                    if(continu==1) // si on atteint le nombre limite de lectures et le bloc est plein,
                        /**nous allouons un nouveau bloc pour éviter les décalages inter-blocs**/
                        //x doit être inséré dans un nouveau bloc i+1 à la position 0
                    {
                        n = alloc_bloc_LOF(fichier,&buf2);
                        buf2.suiv=buf.suiv;
                        buf.suiv = n;
                        ecrireDir_LOF(fichier,i,&buf);
                        (*nb_ecr)++;
                        buf2.t[0] = x; // l'enregistrement qui doit être inséré
                        buf2.nb=1;
                        ecrireDir_LOF(fichier,n,&buf2);
                        (*nb_ecr)++;
                    }/**/

                }
            }
            aff_entete_LOF(fichier,2,Entete_LOF(fichier,2)+1);//incrémenter le nombre d'enregistrements dans le fichier
            return;
        }
    }
    return;
}


/***************Insertion2*********************/

void insertion2_LOF(LOF *fichier, Tenreg_LOF enreg,int *nb_lect_limite, int *nb_lect, int *nb_ecr)
{
    enreg.efface=0;
    int i=-1,j=0,trouv=0,efface=0,k=0,n=0,num_bloc_prec=-1;
    Tbloc_LOF buf,buf2;
    Tenreg_LOF x;
    if (fichier->f!=NULL)
    {
        recherche_LOF(fichier,enreg.cle,&trouv,&efface,&i,&num_bloc_prec,&j,nb_lect,nb_ecr);
        //printf("\n > i=%d , j= %d \n",i,j);
        if(trouv && !efface)//la clé existe et est non supprimé
            return;
        else
        {
            if (trouv && efface)
            {
                //la clé existe et est supprimée
                lireDir_LOF(fichier,i,&buf);
                (*nb_lect)++;
                buf.t[j].efface=0;//repositioné le champ efface à faux
                buf.nb++;
                ecrireDir_LOF(fichier,i,&buf);
                (*nb_ecr)++;
                aff_entete_LOF(fichier,2,Entete_LOF(fichier,2)+1);
                return;

            }
            else
                // la clé n'existe pas dans le fichier, il faut l'insérer à l'emplacement (i,j)
            {
                if((i==-1)&&(Entete_LOF(fichier,1)==-1))//insertion dans un fichier vide
                {
                    i = alloc_bloc_LOF(fichier,&buf);
                    buf.t[0] = enreg;
                    buf.nb=1;
                    ecrireDir_LOF(fichier,i,&buf);
                    (*nb_ecr)++;
                    aff_entete_LOF(fichier,1,i);

                }

                else if((i==-1)&&(Entete_LOF(fichier,1)!=-1))//insertion en fin de fichier
                {
                    i = alloc_bloc_LOF(fichier,&buf2);
                    lireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_lect)++;

                    buf.suiv=i;
                    /**redistribution**/
                    int nb_enr = buf.nb;//nombre d'enregistrements dans le bloc i avant la redistribution
                    j=(buf.nb/2)+1;// la position à partir où on va ramène les enregistrements
                    //du bloc précédent vers le nouveau bloc
                    int cpt=0;
                    for(int k=j; k<nb_enr; k++)
                    {
                        buf2.t[cpt]=buf.t[k];// déplacer l'enregistrement du bloc précédent vers le nouveau bloc
                        //effacer l'enreg du bloc précédent
                        buf.t[k].efface=1;
                        buf.t[k].cle=0;
                        buf.nb--;
                        cpt++;
                    }
                    ecrireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_ecr)++;
                    /*********/
                    buf2.t[cpt] = enreg;
                    buf2.nb=cpt+1;
                    ecrireDir_LOF(fichier,i,&buf2);
                    (*nb_ecr)++;
                }


                else//insertion dans un bloc (au milieu du fichier)
                {
                    int continu=1;
                    int cpt_lect=0;
                    while ((continu==1) && (cpt_lect<*nb_lect_limite) && (i!= -1))
                    {

                        lireDir_LOF(fichier,i,&buf);
                        (*nb_lect)++;
                        cpt_lect++;
                        x = buf.t[buf.nb-1]; // sauvegarder le dernier enregistrement du bloc

                        /** Décalages intra-bloc **/
                        for(k = buf.nb-1; k>j; k--)
                            buf.t[k] = buf.t[k-1];
                        /**/
                        buf.t[j] = enreg;

                        if(x.efface==0)
                        {
                            if(buf.nb < MAX) // si le bloc n'est pas plein, on remet x dans la fin du bloc
                            {
                                buf.nb++;
                                buf.t[buf.nb-1] = x;
                                ecrireDir_LOF(fichier,i,&buf);
                                (*nb_ecr)++;
                                continu = 0;

                            }
                            else // si le bloc est plein, x doit être inséré dans le bloc i+1 à la position 0
                            {
                                /**Décalage inter-blocs**/
                                ecrireDir_LOF(fichier,i,&buf);
                                (*nb_ecr)++;
                                i=buf.suiv;
                                j = 0;
                                enreg = x; // l'enregistrement qui doit être inséré à l'emplacement (i,0)
                            }/**/

                        }
                        else continu=0; //si l'enregistrement est éffacé on le n'insère pas
                    }
                    if(continu==1) // si on atteint le nombre limite de lectures et le bloc est plein,
                        /**nous allouons un nouveau bloc pour éviter les décalages inter-blocs**/
                        //x doit être inséré dans un nouveau bloc i+1 à la position 0
                    {
                        n = alloc_bloc_LOF(fichier,&buf2);
                        buf2.suiv=buf.suiv;
                        buf.suiv = n;

                        /**reditribution du bloc adjacent**/
                        j=(buf.nb/2)+1;// la position à partir où on va ramène les enregistrements
                        //du bloc i vers le nouveau bloc n
                        int cpt=0;
                        /**redistribution**/
                        int nb_enr = buf.nb;//nombre d'enregistrements dans le bloc i avant la redistribution
                        for(int k=j; k<nb_enr; k++)
                        {
                            buf2.t[cpt]=buf.t[k];// déplacer l'enregistrement du bloc i vers le nouveau bloc
                            //effacer l'enreg du bloc i
                            buf.t[k].efface=1;
                            buf.t[k].cle=0;
                            buf.nb--;
                            cpt++;
                        }
                        /**/

                        ecrireDir_LOF(fichier,i,&buf);
                        (*nb_ecr)++;
                        buf2.t[cpt] = x; // insertion de l'enregistrement à la fin du nouveau bloc
                        //après les enregistrements qu'on a ramené du bloc adjacent
                        buf2.nb=cpt+1;// le nombre d'enregistrements dans le nouveau bloc après redistribution
                        ecrireDir_LOF(fichier,n,&buf2);
                        (*nb_ecr)++;
                    }/**/

                }
            }
            aff_entete_LOF(fichier,2,Entete_LOF(fichier,2)+1);//incrémenter le nombre d'enregistrements dans le fichier
            return;
        }
    }
    return;
}

/**--------Procédure de suppression 1 dans un fichier LOF----------**/

void suppression1_LOF(LOF *fichier,Tenreg_LOF enreg,int* nb_lect,int* nb_ecr)
{
    int i,j,trouv,h,num_bloc_prec,efface;
    Tbloc_LOF buf;
    if(fichier->f!=NULL)
    {
        recherche_LOF(fichier,enreg.cle,&trouv,&efface,&i,&num_bloc_prec,&j,nb_lect,nb_ecr);
        if (trouv)
        {
            lireDir_LOF(fichier,i,&buf);// on lis le bloc voulu
            (*nb_lect)++;

            while (j<buf.nb-2)
            {
                // on ecrase la case voulu supprimer
                buf.t[j]=buf.t[j+1];
                j++;
            }
            // on efface le dernier enreg
            buf.t[buf.nb-1].cle=0;
            buf.t[buf.nb-1].efface=1;
            buf.nb-- ;
            aff_entete_LOF(fichier,2,Entete_LOF(fichier,2)-1); // on decremente le nombre d'enregistrements dans le fichier
            if(buf.nb==0)// on test si le bloc est vide si oui on le libère
            {
                if(i==Entete_LOF(fichier,1))
                    aff_entete_LOF(fichier,1,-1);//fichier devient vide
                else
                {
                    h = buf.suiv;
                    lireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_lect)++;

                    buf.suiv = h; // on chaine le bloc precedant avec le bloc suivant
                    ecrireDir_LOF(fichier,num_bloc_prec,&buf);
                    (*nb_ecr)++;

                }

                aff_entete_LOF(fichier,3,Entete_LOF(fichier,3)-1);// on decremente le nombre du blocs dans le fichier
            }
            else
            {
                ecrireDir_LOF(fichier,i,&buf) ; // on ecrit le bloc apres suppression dans le fichier ;
                (*nb_ecr)++;
            }
        }

    }

}


/**--------Procédure de suppression 2 dans un fichier LOF----------**/

void suppression2_LOF(LOF *f,Tenreg_LOF enreg,int *nb_lect,int *nb_ecr)
{
    Tbloc_LOF buf,bufp,bufs;
    int i,j,num_bloc_prec,trouv,efface;
    if(f->f!=NULL)
    {
        recherche_LOF(f,enreg.cle,&trouv,&efface,&i,&num_bloc_prec,&j,nb_lect,nb_ecr);
        if(trouv)
        {
            lireDir_LOF(f,i,&buf);
            (*nb_lect)++;

            while(j<buf.nb-1)
            {
                // on ecrase la case voulu supprimer

                buf.t[j] = buf.t[j+1];
                j++;
            }
            // on efface le dernier enreg
            buf.t[j].cle = 0;
            buf.t[j].efface = 1;
            buf.nb--;

            if (buf.nb == 0 && i!=Entete_LOF(f,1) && i!=Entete_LOF(f,3))//le bloc devient vide, et le bloc n'est pas en début ou à la fin
            {
                lireDir_LOF(f,num_bloc_prec,&bufp);
                (*nb_lect)++;

                lireDir_LOF(f, buf.suiv,&bufs);
                (*nb_lect)++;

                if (bufp.nb == MAX || bufs.nb == MAX)// si l'un des blocs adjacents est plein.
                {
                    /**redistribution**/
                    int posredist = (bufp.nb + bufs.nb)/3;// la position à partir où on va ramène les enregistrements du bloc précédent
                    // et jusquà où on va ramène les enregistrements du bloc suivant
                    // vers le nouveau bloc n
                    int cpt=0;
                    int nbp=bufp.nb;//nombre d'enregistrements dans le bloc précédent avant la redistribution
                    for(int k = posredist; k<nbp; k++)
                    {
                        buf.t[cpt] = bufp.t[k];// déplacer l'enregistrement du bloc précédent vers le nouveau bloc
                        //effacer l'enreg du bloc précédent
                        bufp.t[k].efface=1;
                        //et décrémenter le nombre d'enregistrements
                        bufp.nb--;
                        cpt++;
                    }
                    int nbs=bufs.nb;//nombre d'enregistrements dans le bloc suivant avant la redistribution
                    for(int k = 0; k<posredist-1; k++)
                    {
                        buf.t[cpt] = bufs.t[k];// déplacer l'enregistrement du bloc suivant vers le nouveau bloc
                        //effacer l'enreg du bloc suivant
                        bufs.t[k].efface=1;
                        bufs.nb--;
                        cpt++;

                    }
                    int cpt2=0;
                    for(int k = posredist-1; k<nbs; k++)
                    {
                        //décalages des enregistrements dans le bloc suivant après la redistribution
                        bufs.t[cpt2] = bufs.t[k];
                        cpt2++;
                    }
                    bufp.nb = posredist;//nombre d'enregistrements dans le bloc précédent après la redistribution
                    bufs.nb = bufs.nb-posredist+1;//nombre d'enregistrements dans le bloc suivant après la redistribution
                    buf.nb = cpt;//nombre d'enregistrements dans le bloc i après la redistribution
                    ecrireDir_LOF(f,num_bloc_prec,&bufp);
                    (*nb_ecr)++;

                    ecrireDir_LOF(f,buf.suiv,&bufs);
                    (*nb_ecr)++;

                }
            }
            ecrireDir_LOF(f,i,&buf);
            (*nb_ecr)++;
            // on decremente le nombre d'enregistrements dans le fichier
            aff_entete_LOF(f,2,Entete_LOF(f,2)-1);
        }
    }
}


/***************    affichage LOF   *********************/

void affichage_LOF (LOF *fichier)
{
    int i,j,n;
    Tbloc_LOF buf;
    if(fichier->f!=NULL)//si le fichier existe
    {
        i=Entete_LOF(fichier,1);//recupérer le numéro du premier bloc
        n=Entete_LOF(fichier,2);//recupérer le nombre d'enregistrements

        if (n!=0)//si le fichier n'est pas vide
        {
            printf("\n\t\tAFFICHAGE entete_LOF");
            printf("\n\t***N° 1bloc=%d",Entete_LOF(fichier,1));
            printf("\n\t***NbEnreg=%d",Entete_LOF(fichier,2));
            printf("\n\t***N°0 dernier bloc=%d\n",Entete_LOF(fichier,3));

            printf("\t\t\t==============\n");
            printf("\t\t\t  L'Affichage  \n");
            printf("\t\t\t==============\n");
            printf("\t\t+----------+----------+\n");
            printf("\t\t|  La cle  |          |\n");
            printf("\t\t+----------+----------+\n");
            while(i!=-1)//tant qu'on est pas arrivé à la fin de fichier
            {
                lireDir_LOF(fichier,i,&buf);//lire le buf i

                if (buf.nb!=0)
                {
                    printf("\n i=%d|nbenr=%d|suiv=%d",i,buf.nb,buf.suiv);
                    printf(" [ ");

                    for (j=0; j<buf.nb; j++) //parcourir tout le tableau
                    {
                        if (buf.t[j].efface==0)//chercher les positions non éffacées pour les afficher
                        {
                            printf(" | %lld | ",buf.t[j].cle);
                        }
                    }
                    printf(" ] \n");

                    printf("\t\t+----------+----------+\n");
                }
                i=buf.suiv;//aller au prochain bloc
            }
        }
        else printf("\n \t\t<<Le fichier est vide>>");
    }
}

/**********************affichage d'entete********************/

void affichage_entete_LOF(LOF *fichier)//afficher les caracteristiques de fichier
{
    printf("\n\t\tAFFICHAGE entete_LOF");
    printf("\n\t***Numero du premier bloc=%d",Entete_LOF(fichier,1));
    printf("\n\t***Le nombre des enregistrements=%d",Entete_LOF(fichier,2));
    printf("\n\t***Numero du dernier bloc=%d",Entete_LOF(fichier,3));
}

/**********************Chargement initiale LOF ********************/
/*
void Chargement_Initial_LOF( LOF *f, int n, float u )
// u est un réel dans ]0,1] et désigne le taux de chargement voulu au départ
{
    Tbloc_LOF buf,buf2;
    int i,j,k;
    int e;
    i = 0;
    aff_entete_LOF(f,1,i);
// num de bloc à remplir
    j = 0;
// num d'enreg dans le bloc
    printf( "Donner les enregistrements en ordre croissant suivant la clé : ");
    for(k=0; k<n; k++)
    {
        scanf("%d",&e);
        if ( j < u*buf.nb )
        {
// ex: si u=0.5, on remplira les bloc jusqu'à b/2 enreg
            buf.t[j].cle = e ;
            j = j+1;
        }
        else
        {
// j > u*b : buf doit être écrit sur disque
            buf.nb = j;
            int h=alloc_bloc_LOF(f,&buf2);
            buf.suiv=h;
            ecrireDir_LOF( f, i, &buf );
            buf=buf2;
            buf.t[0].cle = e; // le kème enreg sera placé dans le prochain bloc, à la position 1
            i = i+1 ;
            j = 1;
        }

// à la fin de la boucle, il reste des enreg dans buf qui n'ont pas été sauvegardés sur disque
        buf.nb = j;
        ecrireDir_LOF( f, i, &buf );
// mettre à jour l'entête (le num du dernier bloc et le compteur d'insertions)
        aff_entete_LOF(f,2,n);
        aff_entete_LOF(f,3,i);
    } // chargement-initial
}

/********************** Chargement initiale TOF ********************/
/*
void Chargement_Initial_TOF( TOF *f, int n, float u )
// u est un réel dans ]0,1] et désigne le taux de chargement voulu au départ
{
    Tbloc_TOF buf;
    int i,j,k;
    int e;
    i = 0;
// num de bloc à remplir
    j = 0;
// num d'enreg dans le bloc
    printf( "Donner les enregistrements en ordre croissant suivant la clé : ");
    for(k=0; k<n; k++)
    {
        scanf("%d",&e);
        if ( j < u*buf.nb )
        {
// ex: si u=0.5, on remplira les bloc jusqu'à b/2 enreg
            buf.tab[ j ].cle = e ;
            j = j+1;
        }
        else
        {
// j > u*b : buf doit être écrit sur disque
            buf.nb = j;
            ecrireDir_TOF( f, i, &buf );
            buf.tab[0].cle = e; // le kème enreg sera placé dans le prochain bloc, à la position 1
            i= i+1;
            j= 1;
        }
// à la fin de la boucle, il reste des enreg dans buf qui n'ont pas été sauvegardés sur disque
        buf.nb = j;
        ecrireDir_TOF( f, i, &buf );
// mettre à jour l'entête (le num du dernier bloc et le compteur d'insertions)
        aff_entete_TOF(f,1,i);
        aff_entete_TOF(f,2,n);
    } // chargement-initial
}
*/
/*******************************************************************************
 |      Modules pour Tests et configuration     |
 ******************************************************************************/


/** Structure des 100000 enregistrements à inserer ou supprimer **/
typedef struct
{
    long long cle; // numéro de clé
    char op;  // l'opération à exécuter ‘I’nsertion ou ‘S’uppression.

} enreg;

/** module qui génére l'opération à exécuter aléatoirement **/

char I_ou_S()
{
    char* choix ="IS";
    char ch=choix[rand() % 2];
    return ch;
}

/** module qui génére la clé aléatoirement **/

long long aleacle()
{
    char* numero = "0123456789";
    char cle[6];
    int k;
    for (k=0; k<5; k++)
        cle[k] = numero[rand() % 10];
    cle[5] = '\0';

    return atoll(cle);
}

/**--------Procédure de creation du fichier texte----------**/
//Prépare un fichier texte contenant nbenreg enregistrements générés avec des clés aléatoires
//et une opération à exécuter ‘I’nsertion ou ‘S’uppression.

void creation(FILE* f,char *nomf, int nbenreg)
{
    f = fopen(nomf,"wb+");
    srand(time(NULL));
    enreg e;
    for(int i=0; i<nbenreg; i++)
    {
        e.cle=aleacle();
        e.op= I_ou_S();
        char* ch;
        fwrite(&e,sizeof(enreg),1,f);
    }
    fclose(f);
}
/**--------Procédure d'affichage du fichier texte----------**/

void affich(FILE* f)
{
    printf("\nFichier de 100000 enregistrements:\n");
    f = fopen("enreg_oper.txt","rb+");
    enreg e;
    while((fread(&e,sizeof(enreg),1,f))!=0)
    {
        printf(" | %lld | %c |\n",e.cle,e.op);

    }
    printf("\n Fin de fichier \n");

    fclose(f);

}

/*******************************************************************************
 |      Programmes principal     |
 |    Tests et configuration     |
 ******************************************************************************/

int main()
{
    /*
        LOF* fLOF= ouvrir_LOF("fLOF.txt",'N');
        fermer_LOF(fLOF);
        fLOF= ouvrir_LOF("fLOF.txt",'A');

        Tenreg_LOF e;

        int nb_lect=0, nbecr=0;
        int cas, num_bloc;
        e.cle=2030;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        e.cle=2040;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        e.cle=2020;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        e.cle=2050;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
        e.cle=2060;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        e.cle=2045;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
        e.cle=2010;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);


        e.cle=2047;
        insertion2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        e.cle=2040;
        suppression2_LOF(fLOF,e,&nb_lect,&nbecr);
        affichage_LOF(fLOF);
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);


        /*        nb_lect=0;
                nbecr=0;
                supperssion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
                affichage_LOF(fLOF,"fLOF.txt");
                printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
        */
    //  fermer_LOF(fLOF);
    /*
    nb_lect=0;
    nbecr=0;
        e.cle=2020;
        insertion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
        affichage_LOF(fLOF,"fLOF.txt");
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
    /*
    nb_lect=0;
    nbecr=0;
        e.cle=2030;
        insertion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
        affichage_LOF(fLOF,"fLOF.txt");
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

        nb_lect=0;
    nbecr=0;
        e.cle=2023;
        insertion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
        affichage_LOF(fLOF,"fLOF.txt");
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
    *
            nb_lect=0;
    nbecr=0;
        e.cle=2022;
        supperssion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
        affichage_LOF(fLOF,"fLOF.txt");
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
    /*
                nb_lect=0;
    nbecr=0;
        e.cle=2022;
        insertion2_LOF(fLOF,"fLOF.txt",e,&nb_lect,&nbecr);
        affichage_LOF(fLOF,"fLOF.txt");
        printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);
    */

    //  fermer_LOF(fLOF);
    /*
           TOF* fTOF = ouvrir_TOF("fTOF.txt",'N');

               int nb_lect=0;
               int nbecr=0;
               Tenreg_TOF ee;
               ee.cle=2020;
               insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
               affichage_TOF(fTOF);
               printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

               nb_lect=0;
               nbecr=0;
               ee.cle=2030;
               insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
               affichage_TOF(fTOF);
               printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

               nb_lect=0;
               nbecr=0;
               ee.cle=2022;
               insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
               affichage_TOF(fTOF);
               printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

               nb_lect=0;
               nbecr=0;
               suppression_TOF(fTOF,ee,&nb_lect,&nbecr);
               ee.cle=2022;
               affichage_TOF(fTOF);
               printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

               nb_lect=0;
               nbecr=0;
               ee.cle=2022;
               insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
               affichage_TOF(fTOF);
               printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

               fermer_TOF(fTOF);

    */
    /*
             FILE* fichier;
                     creation(fichier,"enreg_oper.txt",100000);

                 TOF* fTOF = ouvrir_TOF("fTOF.txt",'N');
                 Tenreg_TOF enrTOF;
                     fichier = fopen("enreg_oper.txt","rb+");
                     enreg e;
                 int nb_lect=0, nbecr=0;

                     while((fread(&e,sizeof(enreg),1,fichier))!=0)
                     {
                 int nb_lect=0, nbecr=0;

                         enrTOF.cle=e.cle;
                         if(e.op=='I')
                                        insertion_TOF(fTOF,enrTOF,&nb_lect,&nbecr);
                         if(e.op=='S')

                             suppression_TOF(fTOF,enrTOF,&nb_lect,&nbecr);
             }
             affichage_TOF(fTOF);
             fermer_TOF(fTOF);

    /*
                 int nb_lect=0, nbecr=0;
                 ee.cle=2022;
                 insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 nb_lect=0;
                 nbecr=0;
                 ee.cle=2020;
                 insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 nb_lect=0;
                 nbecr=0;
                 ee.cle=2030;
                 insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 nb_lect=0;
                 nbecr=0;
                 ee.cle=2022;
                 suppression_TOF(fTOF,ee,&nb_lect,&nbecr);
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 nb_lect=0;
                 nbecr=0;
                 insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
                 ee.cle=2017;
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 nb_lect=0;
                 nbecr=0;
                 ee.cle=2040;
                 insertion_TOF(fTOF,ee,&nb_lect,&nbecr);
                 affichage_TOF(fTOF);
                 printf("\nnblect=%d nbecr=%d\n",nb_lect,nbecr);

                 fermer_TOF(fTOF);
           */

    FILE* fichier;
    creation(fichier,"enreg_oper.txt",50);
    affich(fichier);

    //le contenu est sauvegardé dans des fichiers texte qui respectent le format
    //CSV sous le nom de la combinaison (Insertion_1/2_et_Suppression_1/2).
    FILE* fcsv11 =fopen("Insertion_1_et_Suppression_1.csv","w+");
    FILE* fcsv22 =fopen("Insertion_2_et_Suppression_2.csv","w+");
    FILE* fcsv12 =fopen("Insertion_1_et_Suppression_2.csv","w+");
    FILE* fcsv21 =fopen("Insertion_2_et_Suppression_1.csv","w+");
    fprintf(fcsv11,"Nombres de lectures en TOF, Nombres d'écritures en TOF, Nombres de lectures en LOF 11, Nombres d'écritures en LOF 11\n");
    fprintf(fcsv22,"Nombres de lectures en TOF, Nombres d'écritures en TOF, Nombres de lectures en LOF 22, Nombres d'écritures en LOF 22\n");
    fprintf(fcsv12,"Nombres de lectures en TOF, Nombres d'écritures en TOF, Nombres de lectures en LOF 12, Nombres d'écritures en LOF 12\n");
    fprintf(fcsv21,"Nombres de lectures en TOF, Nombres d'écritures en TOF, Nombres de lectures en LOF 21, Nombres d'écritures en LOF 21\n");


    TOF* fTOF = ouvrir_TOF("fTOF.txt",'N');
    LOF* fLOF_I1_S1= ouvrir_LOF("fLOF_ins1_supp1.txt",'N');
    LOF* fLOF_I2_S2= ouvrir_LOF("fLOF_ins2_supp2.txt",'N');
    LOF* fLOF_I1_S2= ouvrir_LOF("fLOF_ins1_supp2.txt",'N');
    LOF* fLOF_I2_S1= ouvrir_LOF("fLOF_ins2_supp1.txt",'N');

    enreg e;
    Tenreg_TOF enrTOF;
    enrTOF.cle=0;
    enrTOF.efface=0;
    Tenreg_LOF enrLOF;
    enrLOF.cle=0;
    enrLOF.efface=0;

    int nblect_TOF=0;
    int nbecr_TOF=0;
    int nblect_LOF=0;
    int nbecr_LOF=0;

    int nb_lect_limite=3;

    fichier = fopen("enreg_oper.txt","rb+");

    while((fread(&e,sizeof(enreg),1,fichier))!=0)
    {

        /* Créer les deux fichiers TOF et LOF en mesurant pour chaque insertion et suppression le facteur
        de chargement et le nombre d’accès disque en distinguant les lectures des écritures. */

        /*  le résultat obtenu est enregistré pour chaque exécution dans les fichiers CSV */

        enrTOF.cle=e.cle;
        enrLOF.cle=e.cle;
        nblect_TOF=0;
        nbecr_TOF=0;
        nblect_LOF=0;
        nbecr_LOF=0;
        if(e.op=='I')//Insertion
        {
            insertion_TOF(fTOF,enrTOF,&nblect_TOF,&nbecr_TOF);

            insertion1_LOF(fLOF_I1_S1,enrLOF,&nb_lect_limite,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv11,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            insertion1_LOF(fLOF_I1_S2,enrLOF,&nb_lect_limite,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv12,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            insertion2_LOF(fLOF_I2_S1,enrLOF,&nb_lect_limite,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv21,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            insertion2_LOF(fLOF_I2_S2,enrLOF,&nb_lect_limite,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv22,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

        }
        if(e.op=='S')//Suppression
        {
            suppression_TOF(fTOF,enrTOF,&nblect_TOF,&nbecr_TOF);

            suppression1_LOF(fLOF_I1_S1,enrLOF,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv11,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            suppression1_LOF(fLOF_I2_S1,enrLOF,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv21,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            suppression2_LOF(fLOF_I1_S2,enrLOF,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv12,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

            nblect_LOF=0;
            nbecr_LOF=0;
            suppression2_LOF(fLOF_I2_S2,enrLOF,&nblect_LOF,&nbecr_LOF);
            fprintf(fcsv22,"%d, %d, %d, %d\n", nblect_TOF, nbecr_TOF, nblect_LOF, nbecr_LOF);

        }


    }
    affichage_TOF(fTOF);

    affichage_LOF(fLOF_I1_S1);

    affichage_LOF(fLOF_I2_S2);

    affichage_LOF(fLOF_I1_S2);

    affichage_LOF(fLOF_I2_S1);

    fclose(fichier);
    fclose(fcsv11);
    fclose(fcsv12);
    fclose(fcsv21);
    fclose(fcsv22);
    fermer_TOF(fTOF);
    fermer_LOF(fLOF_I1_S1);
    fermer_LOF(fLOF_I2_S2);
    fermer_LOF(fLOF_I1_S2);
    fermer_LOF(fLOF_I2_S1);

    return 0;
}
