#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <queue>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Proceso {
    string id;          // Nombre del proceso
    int ti;             // Tiempo de llegada
    int t;              // Tiempo de ejecución
    int tf;             // Tiempo de finalización
    int T;              // Tiempo de retorno
    int E;              // Tiempo de espera
    float I;            // Índice de penalización
    int tiempoRestante; // Tiempo restante para completar el proceso
};

void loadData(const string &filename, vector<Proceso> &procesos);
void roundRobin(vector<Proceso> &procesos, int quantum, bool mostrarTabla);
void fifo(vector<Proceso> &procesos, bool mostrarTabla);
void lifo(vector<Proceso> &procesos, bool mostrarTabla);
void mostrarTablaProcesos(const vector<Proceso> &procesos, const string &algoritmo);

int main() {
    vector<Proceso> procesos;
    char opcion;

    // Cargar datos desde el archivo CSV
    loadData("prueba.csv", procesos);

    cout << "Desea ver las tablas completas de procesos? (s/n): ";
    cin >> opcion;
    bool mostrarTablas = (opcion == 's' || opcion == 'S');

    // Algoritmo Round Robin
    int quantum;
    cout << "Ingrese el quantum para el algoritmo Round Robin: ";
    cin >> quantum;
    
    auto inicioRR = high_resolution_clock::now();
    roundRobin(procesos, quantum, mostrarTablas);
    auto finRR = high_resolution_clock::now();
    auto duracionRR = duration_cast<microseconds>(finRR - inicioRR);
    cout << "Tiempo de ejecucion Round Robin: " << duracionRR.count() << " microsegundos\n\n";

    // Algoritmo FIFO
    auto inicioFIFO = high_resolution_clock::now();
    fifo(procesos, mostrarTablas);
    auto finFIFO = high_resolution_clock::now();
    auto duracionFIFO = duration_cast<microseconds>(finFIFO - inicioFIFO);
    cout << "Tiempo de ejecucion FIFO: " << duracionFIFO.count() << " microsegundos\n\n";

    // Algoritmo LIFO
    auto inicioLIFO = high_resolution_clock::now();
    lifo(procesos, mostrarTablas);
    auto finLIFO = high_resolution_clock::now();
    auto duracionLIFO = duration_cast<microseconds>(finLIFO - inicioLIFO);
    cout << "Tiempo de ejecucion LIFO: " << duracionLIFO.count() << " microsegundos\n";

    return 0;
}

void loadData(const string &filename, vector<Proceso> &procesos) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << filename << endl;
        exit(1);
    }

    string line;
    char delimiter = ',';

    while (getline(file, line)) {
        string id, c1, c2;
        stringstream ss(line);
        getline(ss, id, delimiter);
        getline(ss, c1, delimiter);
        getline(ss, c2, delimiter);

        try {
            Proceso proceso;
            proceso.id = id;
            proceso.ti = stoi(c1);
            proceso.t = stoi(c2);
            proceso.tf = proceso.T = proceso.E = 0;
            proceso.I = 0;
            procesos.push_back(proceso);
        } catch (const invalid_argument &) {
            cerr << "Error: Datos inválidos en la línea: " << line << endl;
        }
    }
    file.close();
}

void mostrarTablaProcesos(const vector<Proceso> &procesos, const string &algoritmo) {
    cout << "Resultados para " << algoritmo << ":\n";
    cout << "+------------------------------------------------+\n";
    cout << "| Proceso |  ti |  t  |  tf |  T  |  E  |    I   |\n";
    cout << "+------------------------------------------------+\n";

    for (const auto &proceso : procesos) {
        cout << "| " << setw(7) << proceso.id
             << " | " << setw(3) << proceso.ti
             << " | " << setw(3) << proceso.t
             << " | " << setw(3) << proceso.tf
             << " | " << setw(3) << proceso.T
             << " | " << setw(3) << proceso.E
             << " | " << setw(6) << fixed << setprecision(4) << proceso.I
             << " |\n";
    }
    cout << "+------------------------------------------------+\n";
}

void fifo(vector<Proceso> &procesos, bool mostrarTabla) {
    int clk = 0; // Reloj
    size_t procesados = 0;
    vector<bool> completados(procesos.size(), false);
    vector<Proceso> resultados = procesos;

    while (procesados < procesos.size()) {
        int idx = -1; //idx es para almacenar el índice del proceso a ejecutar

        // Buscar el primer proceso disponible en orden de llegada
        for (size_t i = 0; i < procesos.size(); ++i) {
            if (!completados[i] && procesos[i].ti <= clk) {
                idx = i;
                break;
            }
        }

        if (idx == -1) {
            int siguiente_ti = INT_MAX;
            for (size_t i = 0; i < procesos.size(); ++i) {
                if (!completados[i] && procesos[i].ti > clk) {
                    siguiente_ti = min(siguiente_ti, procesos[i].ti);
                }
            }
            clk = siguiente_ti;
            continue;
        }

        // Procesar el proceso seleccionado
        completados[idx] = true; 
        resultados[idx].tf = clk + procesos[idx].t;
        resultados[idx].T = resultados[idx].tf - procesos[idx].ti;
        resultados[idx].E = resultados[idx].T - procesos[idx].t;
        resultados[idx].I = (resultados[idx].T > 0) ? float(procesos[idx].t) / resultados[idx].T : 0;

        clk = resultados[idx].tf;
        procesados++;
    }

    if (mostrarTabla) {
        mostrarTablaProcesos(resultados, "FIFO");
    }

    // Calcular y mostrar promedios
    float totalT = 0, totalE = 0, totalI = 0;
    for (const auto &proceso : resultados) {
        totalT += proceso.T;
        totalE += proceso.E;
        totalI += proceso.I;
    }
    cout << "FIFO - Promedios: T=" << totalT / procesos.size()
         << ", E=" << totalE / procesos.size()
         << ", I=" << totalI / procesos.size() << endl;
}

void roundRobin(vector<Proceso> &procesos, int quantum, bool mostrarTabla) {
    int clk = 0;
    size_t procesosTerminados = 0;
    vector<int> tiemposRestantes(procesos.size());
    vector<Proceso> resultados = procesos;

    for (size_t i = 0; i < procesos.size(); ++i) {
        tiemposRestantes[i] = procesos[i].t;
    }

    while (procesosTerminados < procesos.size()) {
        bool procesoEjecutado = false;

        for (size_t i = 0; i < procesos.size(); ++i) {
            if (procesos[i].ti <= clk && tiemposRestantes[i] > 0) {
                int tiempoEjecutado = min(quantum, tiemposRestantes[i]);
                tiemposRestantes[i] -= tiempoEjecutado;
                clk += tiempoEjecutado;
                procesoEjecutado = true;

                if (tiemposRestantes[i] == 0) {
                    resultados[i].tf = clk;
                    resultados[i].T = resultados[i].tf - procesos[i].ti;
                    resultados[i].E = resultados[i].T - procesos[i].t;
                    resultados[i].I = float(procesos[i].t) / resultados[i].T;
                    procesosTerminados++;
                }
            }
        }

        if (!procesoEjecutado) {
            int siguiente_ti = INT_MAX;
            for (size_t i = 0; i < procesos.size(); ++i) {
                if (tiemposRestantes[i] > 0) {
                    siguiente_ti = min(siguiente_ti, procesos[i].ti);
                }
            }
            clk = max(clk, siguiente_ti);
        }
    }

    if (mostrarTabla) {
        mostrarTablaProcesos(resultados, "Round Robin");
    }

    float totalT = 0, totalE = 0, totalI = 0;
    for (const auto &proceso : resultados) {
        totalT += proceso.T;
        totalE += proceso.E;
        totalI += proceso.I;
    }
    cout << "Round Robin - Promedios: T=" << totalT / procesos.size()
         << ", E=" << totalE / procesos.size()
         << ", I=" << totalI / procesos.size() << endl;
}

void lifo(vector<Proceso> &procesos, bool mostrarTabla) {
    int clk = 0;
    size_t procesados = 0;
    vector<bool> completados(procesos.size(), false);
    vector<Proceso> resultados = procesos;

    while (procesados < procesos.size()) {
        int idx = -1;

        for (int i = procesos.size() - 1; i >= 0; --i) {
            if (!completados[i] && procesos[i].ti <= clk) {
                idx = i;
                break;
            }
        }

        if (idx == -1) {
            int siguiente_ti = INT_MAX;
            for (size_t i = 0; i < procesos.size(); ++i) {
                if (!completados[i] && procesos[i].ti > clk) {
                    siguiente_ti = min(siguiente_ti, procesos[i].ti);
                }
            }
            clk = siguiente_ti;
            continue;
        }

        completados[idx] = true;
        resultados[idx].tf = clk + procesos[idx].t;
        resultados[idx].T = resultados[idx].tf - procesos[idx].ti;
        resultados[idx].E = resultados[idx].T - procesos[idx].t;
        resultados[idx].I = (resultados[idx].T > 0) ? float(procesos[idx].t) / resultados[idx].T : 0;

        clk = resultados[idx].tf;
        procesados++;
    }

    if (mostrarTabla) {
        mostrarTablaProcesos(resultados, "LIFO");
    }

    float totalT = 0, totalE = 0, totalI = 0;
    for (const auto &proceso : resultados) {
        totalT += proceso.T;
        totalE += proceso.E;
        totalI += proceso.I;
    }
    cout << "LIFO - Promedios: T=" << totalT / procesos.size()
         << ", E=" << totalE / procesos.size()
         << ", I=" << totalI / procesos.size() << endl;
}