#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int num_fumadores = 3;

class Estanco : public HoareMonitor {
private:
	int mostrador;
	CondVar mostr_vacio;
	CondVar is_ingrediente[num_fumadores];

public:
	Estanco();
	void ponerIngrediente(int ingr);
	void esperar();
	void recogerIngrediente(int ingr);

};

Estanco::Estanco() {
	mostrador = -1;
	mostr_vacio = newCondVar();

	for(int i = 0; i < num_fumadores; i++)
		is_ingrediente[i] = newCondVar();
}

void Estanco::ponerIngrediente(int ingr) {
	mostrador = ingr;
	is_ingrediente[ingr].signal();
}

void Estanco::esperar() {
	if(mostrador != -1)
		mostr_vacio.wait();
}

void Estanco::recogerIngrediente(int ingr) {
	if(mostrador != ingr)
		is_ingrediente[ingr].wait();

	mostrador = -1;
	mostr_vacio.signal();
}

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

int producir_ingrediente() {
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_produ( aleatorio<10, 100>() );

	// informa de que comienza a producir
	cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
	this_thread::sleep_for( duracion_produ );

	const int num_ingrediente = aleatorio < 0, num_fumadores - 1 > () ;

	// informa de que ha terminado de producir
	cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

	return num_ingrediente ;
}

void fumar( int num_fumador ) {

	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_fumar( aleatorio<20, 200>() );

	// informa de que comienza a fumar

	cout << "Fumador " << num_fumador << "  :"
	     << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar

	cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

void funcion_hebra_fumador(MRef<Estanco>monitor, int num_fumador) {
	while(true)
	{
		monitor->recogerIngrediente(num_fumador);
		cout << "Retirado ingrediente: " << num_fumador << endl;
		fumar(num_fumador);
	}
}

void funcion_hebra_estanquero(MRef<Estanco>monitor) {
	int i;

	while(true)
	{
		i = producir_ingrediente();
		monitor->ponerIngrediente(i);
		cout << "Puesto ingrediente: " << i << endl;
		monitor->esperar();
	}
}

int main() {
	cout << "--------------------------" << endl
        << "Problema de los fumadores." << endl
        << "--------------------------" << endl
        << flush ;

   MRef<Estanco>monitor = Create<Estanco>();

   thread hebra_estanco(funcion_hebra_estanquero,monitor),
	  hebra_fumador[num_fumadores];

   for(int i = 0; i < num_fumadores; i++)
         hebra_fumador[i] = thread(funcion_hebra_fumador,monitor,i);

   hebra_estanco.join();

   for(int i = 0; i < num_fumadores; i++)
   	hebra_fumador[i].join();
}

