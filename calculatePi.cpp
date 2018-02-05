# include <mpi.h>
# include <cstdlib>
# include <sstream>
# include <string>
# include <fstream>
# include <iomanip>

# include <chrono>
# include <random>
// Attention , ne marche qu 'en C++ 11 ou supérieur :
unsigned long approximate_pi ( unsigned long nbSamples) {
	/*typedef std::chrono::high_resolution_clock myclock ;
	myclock::time_point beginning = myclock::now();
	myclock::duration d = myclock::now() - beginning ;
	unsigned seed = d.count();
	std::default_random_engine generator(seed) ;*/
	std::random_device generator;
	std::uniform_real_distribution <double>  distribution ( -1.0 ,1.0) ;
	unsigned long nbDarts = 0 ;
	// Throw nbSamples darts in the unit square [-1 :1] x [-1 :1]
	for ( unsigned sample = 0 ; sample < nbSamples ; ++ sample ) {
		double x = distribution ( generator ) ;
		double y = distribution ( generator ) ;
		// Test if the dart is in the unit disk
		if ( x*x+y*y <= 1 ) nbDarts ++ ;
	}
	// Number of nbDarts throwed in the unit disk
	return nbDarts;
}

// Attention , ne marche qu 'en C++ 11 ou supérieur :

int main( int nargs, char* argv[] )
{
	// On initialise le contexte MPI qui va s'occuper :
	//    1. Créer un communicateur global, COMM_WORLD qui permet de gérer
	//       et assurer la cohésion de l'ensemble des processus créés par MPI;
	//    2. d'attribuer à chaque processus un identifiant ( entier ) unique pour
	//       le communicateur COMM_WORLD
	//    3. etc...
	MPI_Init( &nargs, &argv );
	// Pour des raisons de portabilité qui débordent largement du cadre
	// de ce cours, on préfère toujours cloner le communicateur global
	// MPI_COMM_WORLD qui gère l'ensemble des processus lancés par MPI.
	MPI_Comm globComm;
	MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
	// On interroge le communicateur global pour connaître le nombre de processus
	// qui ont été lancés par l'utilisateur :
	int nbp;
	MPI_Comm_size(globComm, &nbp);
	// On interroge le communicateur global pour connaître l'identifiant qui
	// m'a été attribué ( en tant que processus ). Cet identifiant est compris
	// entre 0 et nbp-1 ( nbp étant le nombre de processus qui ont été lancés par
	// l'utilisateur )
	int rank;
	MPI_Comm_rank(globComm, &rank);
	// Création d'un fichier pour ma propre sortie en écriture :

	MPI_Request reqs[nbp-1] ; // required variable for non - blocking calls
	MPI_Status stats[nbp-1] ; // required variable for Waitall routine

	std::stringstream fileName;
	fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	std::ofstream output( fileName.str().c_str() );

	output << "I'm the processus " << rank << " on " << nbp << " processes." << std::endl;

	// Initialize parameters
	int nbSamples = 50000000;
	int nbSamplesPerThread = ceil(nbSamples/nbp);
	int totalSamples = nbSamplesPerThread * nbp;
	int rank_master = 0;
	unsigned long nbDarts[nbp];
	unsigned long nbDarts_send;


	if(rank == rank_master){
		// Wait for slaves
		for(int rk = 0; rk < nbp; rk++){
			if(rk != rank_master){
				MPI_Irecv(&nbDarts[rk] , 1, MPI_UNSIGNED_LONG, rk, 0, globComm, &reqs[rk-1]);
			}
		}

		// Calculate Pi in master
		nbDarts_send = approximate_pi(nbSamplesPerThread);
		output << "NbDarts:" << nbDarts_send << std::endl;
		nbDarts[rank_master] = nbDarts_send;

		// Wait for slaves
		MPI_Waitall(nbp-1, reqs, stats);

		// Calculate ratio
		unsigned long totalNbDarts = 0;
		for(int i = 0; i < nbp; i++){
			totalNbDarts += nbDarts[i];
		}
		double ratio = double(totalNbDarts) / totalSamples;
		output << "Master: approximate Pi:" << ratio * 4.0 << std::endl;
	}else{
		// Calculate Pi in slaves
		nbDarts_send = approximate_pi(nbSamplesPerThread);
		output << "NbDarts:" << nbDarts_send << std::endl;
		// Send result to master
		MPI_Send(&nbDarts_send, 1, MPI_UNSIGNED_LONG, rank_master, 0, globComm);
	}

    output.close();
	// A la fin du programme, on doit synchroniser une dernière fois tous les processus
	// afin qu'aucun processus ne se termine pendant que d'autres processus continue à
	// tourner. Si on oublie cet instruction, on aura une plantage assuré des processus
	// qui ne seront pas encore terminés.
	MPI_Finalize();
	return EXIT_SUCCESS;
}