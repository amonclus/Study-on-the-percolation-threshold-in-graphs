#include "SitePercolation.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cmath>
#include <chrono>
#include <fstream>  // Para manejar archivos

/**
 * Constructor que inicializa la clase con el número de nodos.
 * Inicializa current_p en 0 para comenzar desde la percolación mínima.
 */
SitePercolation::SitePercolation(int numNodos) : uf(numNodos), uf_aux(numNodos + 2), numNodos(numNodos), current_p(0.0), nodoActivo(numNodos, false) {
    // Los supernodos se colocan en las posiciones numNodos y numNodos + 1 en la estructura auxiliar
    superTop = numNodos;
    superBottom = numNodos + 1;
}

/**
 * Inicializa los supernodos conectando los nodos del top y bottom en la estructura auxiliar.
 */
void SitePercolation::initialize_supernodes() {
    int grid_size = sqrt(numNodos);  // Tamaño de la grilla
    // Conectar el superTop a los nodos del top y el superBottom a los nodos del bottom
    for (int i = 0; i < grid_size; ++i) {
        uf_aux.unite(superTop, i);  // Conectar el supertop con los nodos del top
        uf_aux.unite(superBottom, numNodos - grid_size + i);  // Conectar el superbottom con los nodos del bottom
    }
}

/**
 * Genera una configuración de pesos aleatorios para los vértices.
 */
vector<double> SitePercolation::generate_configuration() {
    vector<double> configuracion(numNodos);

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    srand(seed);

    for (int i = 0; i < numNodos; ++i) {
        configuracion[i] = static_cast<double>(rand()) / RAND_MAX;  // Peso aleatorio entre 0 y 1
    }

    return configuracion;
}

/**
 * Realiza una percolación incremental. Solo se procesan los vértices cuyo peso esté entre current_p y el nuevo p.
 * El vector de nodos activos se va actualizando.
 */
int SitePercolation::generate_single_percolation(const vector<Edge>& aristas, const vector<double>& configuracion, double p, int &Smax) {
    if (p < current_p) {
        cerr << "Error: No se puede realizar una percolación con un p menor que el actual." << endl;
        return uf.Ncc(numNodos);  // Devuelve el número actual de componentes si no es válido
    }

    // Activar nodos cuya configuración esté entre current_p y el nuevo p
    for (int i = 0; i < numNodos; ++i) {
        if (!nodoActivo[i] && configuracion[i] <= p) {
            nodoActivo[i] = true;  // Activar el nodo si no lo estaba ya
        }
    }

    // Conectar solo los nodos activos
    for (const auto& arista : aristas) {
        int u = arista.first;
        int v = arista.second;
        if (nodoActivo[u] && nodoActivo[v]) {  // Unir solo nodos activos
            uf.unite(u, v);

            // Actualizar el tamaño del clúster más grande
            int newSmax = max(uf.get_size(arista.first), uf.get_size(arista.second));
            Smax = max(Smax, newSmax);

            uf_aux.unite(u, v);
        }
    }

    // Actualizar el valor actual de p
    current_p = p;

    return uf.Ncc(numNodos);  // Devuelve el número de componentes conexos actual
}

/**
 * Realiza una percolación completa para valores de p entre 0 y 1, y devuelve la relación
 * entre p y el número de componentes conexos.
 */
vector<tuple<double, int, int, double>> SitePercolation::generate_full_percolation(
    const vector<Edge>& aristas, const vector<double>& configuracion, double step, bool visualization) {

    vector<tuple<double, int, int, double>> resultados; // Tupla para p, Ncc, tamaño del clúster más grande, Nmax
    int Smax = 1;  // Inicializa el clúster más grande como 1 (mínimo posible)
    
    // Si se activa la visualización, abrimos los archivos .csv
    ofstream percolation_report, cluster_of_each_node;
    if (visualization) {
        percolation_report.open("percolation_report.csv");
        cluster_of_each_node.open("cluster_of_each_node.csv");
        
        // Escribimos los encabezados en ambos archivos
        percolation_report << "p,Ncc,Smax,Nmax\n";
        cluster_of_each_node << "p";
        for (int i = 0; i < numNodos; ++i) {
            cluster_of_each_node << ",Nodo_" << i;
        }
        cluster_of_each_node << "\n";
    }

    initialize_supernodes();  // Inicializar supernodos

    bool percolation = false;  // Bandera para indicar si se ha producido la percolación

    // Recorremos los valores de p entre 0 y 1 usando el step
    for (double p = 0.0; p <= 1.0 + 1e-10; p += step) {
        int Ncc = generate_single_percolation(aristas, configuracion, p, Smax);

        // Calcular Nmax, que es la fracción de nodos en el clúster más grande
        double Nmax = static_cast<double>(Smax) / numNodos;

        // Almacenar los resultados: p, Ncc, tamaño del clúster más grande, Nmax
        resultados.push_back({p, Ncc, Smax, Nmax});

        // Si se activa la visualización, escribir en los archivos .csv
        if (visualization) {
            // Guardar en el archivo percolation_report.csv
            percolation_report << p << "," << Ncc << "," << Smax << "," << Nmax << "\n";

            // Guardar en el archivo cluster_of_each_node.csv
            cluster_of_each_node << p;
            for (int i = 0; i < numNodos; ++i) {
                cluster_of_each_node << "," << uf.find(i);  // Obtener el clúster del nodo i
            }
            cluster_of_each_node << "\n";
        }

        // Verificar si ya se ha producido la percolación
        if (!percolation && has_percolation()) {
            p_c = p;  // Guardar el valor de p crítico
            percolation = true;
            cout << "Percolación detectada a p = " << p_c << endl;
        }
    }

    // Cerrar los archivos si se abrió para visualización
    if (visualization) {
        percolation_report.close();
        cluster_of_each_node.close();
    }

    return resultados;
}

/**
 * Verifica si se ha producido la percolación, es decir, si existe un camino entre el top y el bottom.
 */
bool SitePercolation::has_percolation() {
    return uf_aux.find(superTop) == uf_aux.find(superBottom);
}
