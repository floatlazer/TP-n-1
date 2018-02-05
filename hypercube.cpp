# include <mpi.h>
# include <cstdlib>
# include <sstream>
# include <string>
# include <fstream>
# include <iomanip>
#include <cmath>

// log2
int logTwo(int power)
{
	if(power <= 0 ) return -1;
	int ret = 0;
	while(power%2 == 0)
	{
		ret++;
		power /= 2;
	}
	if(power != 1) return -2;
	return ret;
}

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
	// l'utilisateur )*
	int d = logTwo(nbp); // Dimension
	if(d < 0)
	{
		std::cout<<"Nomber of process is not power of 2."<<std::endl;
		MPI_Finalize();
		return EXIT_FAILURE;
	}
	int rank;
	MPI_Comm_rank(globComm, &rank);
	// Création d'un fichier pour ma propre sortie en écriture :
	std::stringstream fileName;
	fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	std::ofstream output( fileName.str().c_str() );
	output << "I'm the processus " << rank << " on " << nbp << " processes." << std::endl;

	/* Algorithme
		Every process do d iteration
		In iteration i starting with 0:
			if rank is between 2^i and 2^(i+1)-1, receive token from rank-2^i
			if rank is strictly smaller than 2^i, send token to rank+2^i
	*/
	int token = 0;
	if(rank == 0)
	{
		token = 1248692;
	}
	for(int it = 0; it < d; it++)
	{
		int power2 = pow(2, it);
		if(rank >= power2 && rank <= 2 * power2 - 1 )
		{
			MPI_Recv(&token , 1, MPI_INT, rank - power2, 0, globComm, NULL);
			output << "Recv token " << token << " from node " << rank - power2 << "." << std::endl;
		}
		if(rank < power2)
		{
			MPI_Send(&token, 1, MPI_INT, rank + power2 , 0, globComm);
			output << "Send token " << token << " to node " << rank + power2 << "." << std::endl;
		}
	}
	output.close();
	// A la fin du programme, on doit synchroniser une dernière fois tous les processus
	// afin qu'aucun processus ne se termine pendant que d'autres processus continue à
	// tourner. Si on oublie cet instruction, on aura une plantage assuré des processus
	// qui ne seront pas encore terminés.
	MPI_Finalize();
	return EXIT_SUCCESS;
}