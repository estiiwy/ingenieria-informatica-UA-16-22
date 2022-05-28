#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <queue>
#include <limits>

#include <chrono>


using namespace std;

typedef pair<int, int> Position;

typedef vector<Position> Way;

typedef struct{
        Position current;       // Donde estoy
        ulong way;              // Lo que me ha costado estar donde estoy
        int optimisticValue;    // Estimacion por encima de la solucion de lo que me va a costar
                                // llegar al final.
}Node;

typedef vector< vector<uint> > Problem; // laberinto del minotauro

typedef ulong Solution; //

// El Problem concretamente en esta cota no lo uso.
Solution optimistic_solution(const Node &n, const Problem &p){
	Solution s;
	// s = actual + estimado
        s = n.way + min(n.current.first, n.current.second) + abs(n.current.first - n.current.second);
        //s = n.way.size() + (n.current.first + n.current.second)/ 2;
        return s;
}

Solution pessimistic_solution(const Node &n, const Problem &p){
	int i, j;
	Solution s;
	bool blocked = false;

        s = n.way;		// donde estoy.
	i = n.current.first;
	j = n.current.second;
	while(!blocked && (i != 0 || j != 0)){
		if(p[i][j] == 0){
			blocked = true;
		}
		else{
			if(i == 0){
				if(p[i][j - 1] == 0){
					blocked = true;
				}
				else{
					s++;
					j--;
				}
			}
			else{
				if(j == 0){
					if(p[i - 1][j] == 0){
						blocked = true;
					}
					else{
						s++;
						i--;
					}
				}
				else{
					if(p[i - 1][j - 1] == 1){
						i--;
						j--;
						s++;
					}
					else{
                                                if(p[i][j - 1] == 1){
							j--;
							s++;
						}
						else{
                                                        if(p[i - 1][j] == 1){
								i--;
								s++;
							}
							else{
								blocked = true;		
							}
						}
					}	
				}
			}
		}
	}
	if(i != 0 || j != 0){
		s = numeric_limits<ulong>::max();
	}
	return s;
}

vector<Node> expand(const Node &n){
	vector<Node> hijos;
	Node hijo;
	int p, q;
	for(p = -1; p <= 1; p++){
		for(q = -1; q <= 1; q++){
			if(p != 0 || q != 0){
                                hijo = n;       // copio inicialmente al padre
                                hijo.current.first += p;    // avanzo.
				hijo.current.second += q;
                                hijo.way++;
				hijos.push_back(hijo);
				// la cota optimista y la pesimista solo las calculo si superan isFeasible()
			}
		}
	}
	return hijos;
}

bool is_feasible(const Node &n, const Problem &p, const vector< vector<ulong> > &cas){
        bool fea = false;
        // no me salgo de la matriz
	if(n.current.first >= 0 && n.current.first < p.size() && n.current.second >= 0 && n.current.second < p[0].size()){
                // no estoy en un cero
                if(p[n.current.first][n.current.second] != 0){
                    // soy la que menos he tardado en llegar aqui
                    // esto tambien es poda, de memoria, y no puedo mejorar la solucion desde ese punto
                    // porque tambien hay otro que ha llegado aqui mas rapido.
                    if(n.way < cas[n.current.first][n.current.second]){ /////********
			fea = true;
                    }
		}
	}
	return fea;
}
// para saber si el nodo ya es solucion (estoy en 0 0)
bool is_leaf(const Node &n){
    return n.current.first == 0 && n.current.second == 0;
}

// para saber si la solucion (real o pesimista) actual es mejor que la encontrada hasta el momento.
bool is_better(ulong a, ulong b){
    return a <= b;
}
// para saber si el nodo puede mejorar la mejor solucion encontrada hasta el momento.
// p es la mejor solucion encontrada hasta el momento.
bool is_promising(const Node &a, ulong p){
    return a.optimisticValue <= p;
}

Node initial_node(const Problem &p){
	Node initial;
        initial.current.first = p.size() - 1;
        initial.current.second = p[0].size() - 1;
        initial.way = 1;
        initial.optimisticValue = optimistic_solution(initial, p);
	return initial;
}

ulong solution(const Node &n){
    return n.way;
}


bool operator<(const Node &a, const Node &b){
    return a.optimisticValue > b.optimisticValue;
    // return a.way > b.way;
    //return min(a.current.first, a.current.second) + abs(a.current.first - a.current.second)
    //      < min(b.current.first, b.current.second) + abs(b.current.first - b.current.second);
}

Solution branch_and_bound(Problem p){
        // Matriz para no visitar dos veces la misma celda
        vector< vector<ulong> > costeActualSol(p.size(), vector<ulong>(p[0].size(), numeric_limits<ulong>::max()));

        // current = (n-1, m-1), coste = 1
        Node initial = initial_node(p);

        // Actualizo el coste para llegar a esa celda.
        costeActualSol[p.size() - 1][p[0].size() - 1] = 1;

        // Intento sacar una solucion rapida utilizando el metodo voraz.
        // Inicialmente lo pondria a numeric_limits<ulong>::max()
        Solution current_best = pessimistic_solution(initial, p); //*·*//

        // almaceno el conjunto de nodos vivos (con soluciones parciales que pueden llegar a solucion)
        priority_queue<Node> q; // Hay que sobrecargar el operador < para que funcione la cola de prioridad
                                // con tu tipo de dato.

        //  current = (n-1, m-1), coste = 1 A la cola.
        q.push(initial);

        // mientras queden nodos.
        while(!q.empty()){
                Node n = q.top(); // utiliza el operador < (en este caso saca el nodo que mas cota tenga)
                //cout << "Current best: " << current_best << endl;
                //cout << "Seleccionado(" << n.optimisticValue << "): " << n.current.first << ", " << n.current.second << endl;
                q.pop();
                if(is_leaf(n)){ // n.current.first == 0 && n.current.second == 0
                        if(is_better(solution(n), current_best)){   // if(n.way < current_best){
                                current_best = solution(n);              // current_best = n.way;
			}
		}
                else{
                    // expand(n) devuelve un vector<stl> con el resultado de la expansion del nodo y
                    // el for mejorado lo recorre elemento a elemento
                    for(Node a : expand(n) ){
                            if(is_feasible(a, p, costeActualSol)){
                                    // cout << "(" << a.current.first << ", " << a.current.second << ")" << endl;
                                    costeActualSol[a.current.first][a.current.second] = a.way; // actualizar la celda origen de esta celda con n.
                                    ulong x = pessimistic_solution(n , p);
                                    if(is_better(x, current_best)){
                                            current_best = x;
                                    }
                                    // ****************************************** //
                                    a.optimisticValue = optimistic_solution(a, p);
                                    // si mejora la mejor solucion encontrada hasta el momento
                                    // lo meto en el conjunto de nodos vivos.
                                    if(is_promising(a, current_best)){
                                            q.push(a);
                                    }
                                    //else{
                                    //    cout << "poda********************" << endl;
                                    //}
                            }
                    }
                }
        }
        /*
        for(int i = 0; i < costeActualSol.size(); i++){
            for(int j = 0; j < costeActualSol[0].size(); j++){
                if(costeActualSol[i][j] != numeric_limits<ulong>::max())
                    cout << costeActualSol[i][j] << " ";
                else
                    cout << " - ";
            }
            cout << endl;
        }*/
	return current_best;
}

bool leerFichero(string nf, Problem &laberinto){
        bool leido;
        int filas, columnas;
        ifstream fich;

        leido = false;
        fich.open(nf.c_str());
        if(fich.is_open()){
                leido = true;
                fich >> filas >> columnas;
                laberinto = vector< vector<uint> >(filas, vector<uint>(columnas, 0));
                for(int i = 0; i < filas; i++){
                        for(int j = 0; j < columnas; j++){
                                fich >> laberinto[i][j];
                        }
                }
                fich.close();
        }
        return leido;
}

int main(int argc, char * argv[]){
    Solution s;
    Problem p;
    leerFichero(argv[2], p);
    clock_t total = 0;
    auto start = clock();
    s = branch_and_bound(p);
    auto end = clock();
    total += end - start;
    cout << "CPU elapsed time: ";
    cout << total << endl;
    cout << "Shortest path: " << s << endl;
    return 0;
}
