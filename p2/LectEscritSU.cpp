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

const int num_lectores = 4;
const int num_escritores = 4;

class LectEscritSU : public HoareMonitor {
private:
	bool escrib;
	int n_lec;
	CondVar lectura;
	CondVar escritura;

public:
	LectEscritSU();
	void ini_lectura();
	void ini_escritura();
	void fin_lectura();
	void fin_escritura();
};

LectEscritSU::LectEscritSU() {
	escrib = false;
	n_lec = 0;
	lectura = newCondVar();
	escritura = newCondVar();
}

void LectEscritSU::ini_lectura() {
	if (escrib)
		lectura.wait();

	n_lec++;
	lectura.signal();
}

void LectEscritSU::fin_lectura() {
	n_lec--;
	if (n_lec == 0)
		escritura.signal();
}

void LectEscritSU::ini_escritura() {
	if (n_lec > 0 || escrib)
		escritura.wait();

	escrib = true;
}

void LectEscritSU::fin_escritura() {
	escrib = false;

	if (lectura.get_nwt() != 0)
		lectura.signal();
	else
		escritura.signal();
}

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

void leer(int lector) {
	chrono::milliseconds tiempo_lectura(aleatorio<20, 200>());
	cout << "El lector " << lector << " lee " << tiempo_lectura.count() << " ms" << endl;
	this_thread::sleep_for(tiempo_lectura);
	cout << "El lector " << lector << " ha finalizado" << endl;
}

void escribir(int escritor) {
	chrono::milliseconds tiempo_escritura(aleatorio<30, 300>());
	cout << "El escritor " << escritor << " escribe " << tiempo_escritura.count() << " ms" << endl;
	this_thread::sleep_for(tiempo_escritura);
	cout << "El escritor " << escritor << " ha finalizado" << endl;
}

void funcion_hebra_escritor(MRef<LectEscritSU>monitor, int n) {
	while (1) {
		chrono::milliseconds espera(aleatorio<50, 700>());
		this_thread::sleep_for(espera);
		monitor->ini_lectura();
		leer(n);
		monitor->fin_lectura();
	}
}

void funcion_hebra_lector(MRef<LectEscritSU>monitor, int n) {
	while (1) {
		chrono::milliseconds espera(aleatorio<50, 700>());
		this_thread::sleep_for(espera);
		monitor->ini_escritura();
		escribir(n);
		monitor->fin_escritura();
	}
}

int main() {

	cout << "------------------------------------------------------------------------" << endl
	     << "Problema de los lectores-escritores (4 lect/escrit, Monitor SU)." << endl
	     << "------------------------------------------------------------------------" << endl
	     << flush ;

	MRef<LectEscritSU>monitor = Create<LectEscritSU>();

	thread hebra_lector[num_lectores],
	       hebra_escritor[num_escritores];

	for (int i = 0; i < num_escritores; i++)
		hebra_escritor[i] = thread(funcion_hebra_escritor, monitor, i);

	for (int i = 0; i < num_lectores; i++)
		hebra_lector[i] = thread(funcion_hebra_lector, monitor, i);

	for (int i = 0; i < num_escritores; i++)
		hebra_escritor[i].join();

	for (int i = 0; i < num_lectores; i++)
		hebra_lector[i].join();
}