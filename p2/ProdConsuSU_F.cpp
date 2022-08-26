#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>
#include <mutex>
#include <random>
#include <condition_variable>
#include "HoareMonitor.h"

using namespace HM;
using namespace std;

constexpr int num_items = 40;
mutex mtx;
const int num_prod = 4;
const int num_cons = 4;

unsigned
cont_prod[num_items],
          cont_cons[num_items];

int items_producidos[num_prod];

class ProdConsSU : public HoareMonitor {
private:
	static const int num_celdas_total = 10;
	int buffer[num_celdas_total], items, ind_lect, ind_escr;
	CondVar ocupadas;
	CondVar libres;

public:
	ProdConsSU();
	int extraer();
	void insertar(int x);
};

ProdConsSU::ProdConsSU() {
	ind_escr = 0;
	ind_lect = 0;
	libres = newCondVar();
	ocupadas = newCondVar();
	items = 0;
}

int ProdConsSU::extraer() {
	if (items == 0)
		ocupadas.wait();

	assert(0 < items);
	items--;
	int x = buffer[ind_lect];
	ind_lect++;
	ind_lect %= num_celdas_total;
	libres.signal();
	return x;
}

void ProdConsSU::insertar(int x) {
	if (items == num_celdas_total)
		libres.wait();

	assert(items < num_celdas_total);
	buffer[ind_escr] = x;
	items++;
	ind_escr++;
	ind_escr %= num_celdas_total;
	ocupadas.signal();
}

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

int producir_dato(int i) {
	int aux;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20, 100>() ));
	mtx.lock();
	aux = i * (num_items / num_prod) + items_producidos[i];
	items_producidos[i]++;
	cout << "producido: " << aux << endl << flush ;
	mtx.unlock();
	cont_prod[aux] ++ ;
	return aux;
}

void consumir_dato( unsigned dato ) {

	if ( num_items <= dato )
	{
		cout << " dato === " << dato << ", num_items == " << num_items << endl ;
		assert( dato < num_items );
	}
	cont_cons[dato]++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20, 100>() ));
	mtx.lock();
	cout << "                  consumido: " << dato << endl ;
	mtx.unlock();
}

void ini_contadores() {
	for ( unsigned i = 0 ; i < num_items ; i++ )
	{	cont_prod[i] = 0 ;
		cont_cons[i] = 0 ;
	}
}

void test_contadores() {
	bool ok = true ;
	cout << "comprobando contadores ...." << flush ;

	for ( unsigned i = 0 ; i < num_items ; i++ )
	{
		if ( cont_prod[i] != 1 )
		{
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}
		if ( cont_cons[i] != 1 )
		{
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}
	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

void funcion_hebra_productora(MRef<ProdConsSU>monitor, int x) {
	for ( unsigned i = 0 ; i < (num_items / num_prod) ; i++ ) {
		int valor = producir_dato(x) ;
		monitor->insertar( valor );
	}
}

void funcion_hebra_consumidora(MRef<ProdConsSU>monitor) {
	for ( unsigned i = 0 ; i < (num_items / num_cons); i++ )
	{
		int valor = monitor->extraer();
		consumir_dato( valor ) ;
	}
}

int main() {

	cout << "-------------------------------------------------------------------------------" << endl
	     << "Problema de los productores-consumidores (4 prod/cons, Monitor SU, buffer FIFO). " << endl
	     << "-------------------------------------------------------------------------------" << endl
	     << flush ;

	MRef<ProdConsSU>monitor = Create<ProdConsSU>();
	thread hebra_productora[num_prod],
	       hebra_consumidora[num_cons];

	for (int i = 0; i < num_prod; i++)
		hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);

	for (int i = 0; i < num_cons; i++)
		hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor);

	for (int i = 0; i < num_prod; i++)
		hebra_productora[i].join();

	for (int i = 0; i < num_cons; i++)
		hebra_consumidora[i].join();

	test_contadores();
}