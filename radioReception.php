<?php
/*
Cette page r�cupere les informations du signal radio recu par le raspberry PI et effectue une action
en fonction de ces derni�res.

NB : Cette page est appell�e en parametre du programme C 'radioReception', vous voupvez tout � fait
appeller une autre page en renseignant le parametre lors de l'execution du programme C.

@author : Valentin CARRUESCO (idleman@idleman.fr)
@licence : CC by sa (http://creativecommons.org/licenses/by-sa/3.0/fr/)
RadioPi de Valentin CARRUESCO (Idleman) est mis � disposition selon les termes de la 
licence Creative Commons Attribution - Partage dans les M�mes Conditions 3.0 France.
Les autorisations au-del� du champ de cette licence peuvent �tre obtenues � idleman@idleman.fr.
*/

//R�cuperation des parametres du signal sous forme de variables
list($file,$sender,$group,$state,$interruptor) = $_SERVER['argv'];
//Affichages des valeurs dans la console a titre informatif
echo "\nemetteur : $sender,\n Groupe :$group,\n on/off :$state,\n boutton :$interruptor";

//En fonction de la rang� de bouton sur laquelle on � appuy�, on effectue une action
switch($interruptor){

	//Rang�e d'interrupteur 1 
	case '0':
		system('gpio mode 3 out');
		//Bouton On appuy�
		if($state=='on'){
			echo 'Mise � 1 du PIN 3 (15 Pin physique)';
			// system('gpio write 3 1');
		//Bouton off appuy�
		}else{
			echo 'Mise � 0 du PIN 3 (15 Pin physique)';
			// system('gpio write 3 0');
		}
	break;
	
	//Rang�e d'interrupteur 2
	case '1':
		//Bouton On appuy�
		if($state=='on'){
			//mettre quelque chose ici
		//Bouton off appuy�
		}else{
			//mettre quelque chose ici
		}
	break;
	
	///Rang�e d'interrupteur 3
	case '2':
		if($state=='on'){
			//mettre quelque chose ici	
		}else{
			//mettre quelque chose ici
		}
	break;
}

?>