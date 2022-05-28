// 48788593G - Esteban Antón Sellés

#include <iostream>
#include <vector>
#include <queue>
#include <fstream>
#include <cstring>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <chrono>

using namespace std;

// Errores durante la ejecucíon
enum Error
{
  ERR_OPT,
  ERR_FILE
};

void error (Error n, string x)
{
  cout << "ERROR: ";
  switch (n)
  {
    case ERR_OPT:
      cout << "unknown option " << x << "." << endl <<
              "Usage: " << endl <<
              "maze-bb [-p] -f file" << endl;
      break;
    case ERR_FILE:
      cout << "can't open file: " << x << "." << endl;
      break;
  }
}

// Para pedir los datos de un nodo
enum Node_values
{
  STEPS,    // Pasos recorridos desde la primera casilla hasta su posición
  POS,   // Posición actual
  PREV,   // Posición previa a esta (de dónde vengo)
  OPTIM   // Cota optimista
};


typedef tuple<unsigned long, pair<short, short>, pair<short, short>, unsigned long> Node;

// NO - vector<Node> solution;    // Solución final - Esto mata la memoria en las soluciones grande
unsigned long optimistic_value = 0;   // Cota optimista (Menor que la solución)
unsigned long solution = 1;   // Total de pasos recorridos
priority_queue<Node> queue;   // Cola de prioridad en la que se ordenan los elementos

// CONTADORES
unsigned long long no_promising = 0;    // Nodos no prometedores
unsigned long long no_feasible = 0;     // Nodos no factibles
unsigned long long explored = 0;        // Nodos explorados (Por los que se ha pasado)
unsigned long long expanded = 0;        // Nodos expandidos
unsigned long long created = 0;         // Nodos creados
unsigned long long completed = 0;       // Nodos completados (Han llegado hasta ser hoja)
unsigned long long update_solution = 0; // Veces que se actualiza la solución de una poda pesimista
unsigned long long added = 0;           // Nodos añadidos a la lista de nodos vivos
unsigned long long prom_but_disc = 0;   // Nodos que han sido prometedores pero después se han podado

// CARACTERÍSTICAS LABERINTO
unsigned short n, m;   // Dimensiones del laberinto
vector< vector<short> > maze;    // Laberinto
  // Almacén de costes para llegar a una celda, con lo que no se volverá a visitar una celda ya visitada
vector< vector<unsigned long> > warehouse;


// ## ALGORTIMO VORAZ ##
float distance(const short &i, const short &j)
{
  return abs((m-1)*i - (n-1)*j) / sqrt(pow(m-1, 2) + pow(n-1,2));
}

bool tryMove(const short &i, const short &j)
{
  bool tryIt = false;

  if (i < n && j < m && maze[i][j])
  {
    tryIt = true;
  }

  return tryIt;
}

int calculate(short i, short j)
{
  bool diag = tryMove(i+1, j+1);
  bool right = tryMove(i, j+1);
  bool down = tryMove(i+1, j);
  float dist = FLT_MAX;
  int calc = -2;
  if (diag)
  {
    dist = distance(i+1, j+1);
    calc = 0;
  }
  if (right && dist > distance(i, j+1))
  {
    dist = distance(i, j+1);
    calc = 1;
  }
  if (down && dist > distance(i+1, j))
  {
    calc = -1;
  }

  return calc;
}

unsigned long greedyAlgorithm(const Node &node, bool &blocked)
{
  unsigned long path = 0;
  short i = get<POS>(node).first;
  short j = get<POS>(node).second;
  int move;
  bool end = false;

  // Si la posiciión (0, 0) es 0 es imposible hacer el laberinto
  if (maze[i][j] == 0)
  {
    blocked = true;
  }

  while (!blocked && (i < n || j < m) && !end)
  {
    path++;
    move = calculate(i, j);
    if (move == 0)
    {   // Diagonal
      i++;
      j++;
    }
    else if (move == 1)
    {   // Derecha
      j++;
    }
    else if (move == -1)
    {   // Abajo
      i++;
    }
    else if (i == n-1 && j == m-1)
    {
      end = true;
    }
    else
    {
      blocked = true;
    }
  }
  return path;
}
// ######################################################################

// Solución voraz
unsigned long pessimistic_height(const Node &n)
{
  unsigned long pessimistic = 0;
  bool blocked = false;

  pessimistic = greedyAlgorithm(n, blocked);

  if (blocked)
  {
    pessimistic = numeric_limits<unsigned long>::max();
  }

  return pessimistic;
}

unsigned long optimistic_height(const unsigned long &sol, const short &x, const short &y)
{
  unsigned long optimistic = 0;

  optimistic = sol + sqrt(pow(n - x, 2) + pow(m - y, 2));

  return optimistic;
}

// Es prometedor, da buenas cotas
bool is_promising(const Node &node)
{
  bool promising = true;

  if (get<OPTIM>(node) > solution)
  {   // No es un buen nodo para expandir por sus cotas
    promising = false;
    no_promising++;
  }
  warehouse[get<POS>(node).first][get<POS>(node).second] = get<STEPS>(node);

  return promising;
}

// Es posible, está en un lugar en el que se puede estar
bool is_feasible(const Node &node)
{
  bool feasible = true;

  if (get<POS>(node).first < 0 || get<POS>(node).second < 0 ||
      get<POS>(node).first >= n || get<POS>(node).second >= m)
  {
    feasible = false;
    no_feasible++;
  }
  else if (maze[get<POS>(node).first][get<POS>(node).second] == 0 ||
          get<STEPS>(node) >= warehouse[get<POS>(node).first][get<POS>(node).second])
  {
    feasible = false;
    no_feasible++;
  }

  return feasible;
}

// Comprueba que la solución que se está calculando ahora sea mejor que la calculada hasta ahora
bool is_better(const Node &node)
{
  bool better = false;

  if (get<STEPS>(node) <= solution/* && get<STEPS>(node) > optimistic_value*/)
  {
    better = true;
  }
  /*if (get<STEPS>(node) > optimistic_value && get<STEPS>(node) <= pessimistic_value &&
      get<STEPS>(node) >= solution)
  {
    better = true;
  }*/

  return better;
}

// Comprueba si el nodo es el final del laberinto, si está en (n, m)
bool is_leaf(const Node &node)
{
  bool leaf = false;

  if (get<POS>(node).first == n-1 && get<POS>(node).second == m-1)
  {
    leaf = true;
  }

  return leaf;
}

vector<Node> expand(const Node &node)
{
  expanded++;
  vector<Node> sons;

  for (short i = -1; i < 2; i++)
  {
    for (short j = -1; j < 2; j++)
    {
      if (i != 0 || j != 0)
      {
        sons.push_back(Node(get<STEPS>(node) + 1, make_pair(get<POS>(node).first + i, get<POS>(node).second + j),
                      make_pair(get<POS>(node).first, get<POS>(node).second),
                      optimistic_height(get<STEPS>(node)+1, get<POS>(node).first + i, get<POS>(node).second + j)));
        created++;
      }
    }
  }

  return sons;
}

// Sobrecarga de operador para que la cola se ordene por orden de mejor cota
bool operator<(const Node &a, const Node &b)
{
  return get<OPTIM>(a) > get<OPTIM>(b);
}

// En cada llamada es un nuevo nodo, con lo que vuleve a estudiar todo de nuevo
unsigned long branchAndBoundAlgorithm()
{
  explored++;
  // Inicialización de valores
  warehouse = vector< vector<unsigned long> >(n, vector<unsigned long>(m, numeric_limits<unsigned long>::max()));
  Node initial_node(solution++, make_pair(0, 0), make_pair(0, 0), optimistic_height(solution, 0, 0));
  // Las cotas
  optimistic_value = optimistic_height(get<STEPS>(initial_node), get<POS>(initial_node).first, get<POS>(initial_node).second);
  solution = pessimistic_height(initial_node);
  // Cola de prioridad
  priority_queue<Node> pq;
  pq.push (initial_node);
  // Se actualiza memoización
  warehouse[get<POS>(initial_node).first][get<POS>(initial_node).second] = get<STEPS>(initial_node);

  while (!pq.empty())
  {
    // Se extrae el nodo más prometedor
    Node aux_n = pq.top();
    pq.pop();

    if (is_leaf(aux_n))
    {   // Caso base - Ser una hoja significa el fin del laberinto
      if (is_better(aux_n))
      {
        solution = get<STEPS>(aux_n);
      }
    }
    else
    {   // Sigue expandiendo
      for (Node a : expand(aux_n))
      {
        if (is_feasible(a))
        {   // Si es posible
          warehouse[get<POS>(a).first][get<POS>(a).second] = get<STEPS>(a);
          optimistic_value = optimistic_height(solution, get<POS>(a).first, get<POS>(a).second);
          if (is_promising(a))
          {
            pq.push(a);
            added++;
          }
        }
      }
    }
  }

  return solution;
}

// Imprime el camino
void printPath()
{

}

// Lee argumentos
bool argsOk(int argc, char* argv[], string &filename, bool &path)
{
  bool ok = true;

  if (argc > 1 && argc <= 4)
  {
    for (int i = 1; i < argc && ok; i++)
    {
      if (strcmp(argv[i], "-f") == 0 &&
          i + 1 < argc)
      {
        ok = true;
        filename = argv[i + 1];
        i++;
      }
      else if (strcmp(argv[i], "-p") == 0)
      {
        path = true;
      }
      else
      {
        ok = false;
        error(ERR_OPT, argv[i]);
      }
    }
  }

  return ok;
}

bool readFile(const string &name)
{
  bool readed = true;
  ifstream file;
  file.open(name.c_str());

  if (file.is_open())
  {
    file >> n >> m;
    maze = vector< vector<short> >(n, vector<short>(m, 0));
    for (short i = 0; i < n; i++)
    {
      for (short j = 0; j < m; j++)
      {
        file >> maze[i][j];
      }
    }
  }
  else
  {
    readed = false;
    error(ERR_FILE, name);
  }

  return readed;
}

/* Salidas:
  - Longitud del camino de la solución
*/
int main(int argc, char* argv[])
{
  string filename;    // Nombre del fichero
  bool path;    // Para saber si se quiere mostrar o no el camino
  Node node;    // Nodo
  float time = 0.0;

  if (argsOk(argc, argv, filename, path) && readFile(filename))
  {
    auto start = clock();
    branchAndBoundAlgorithm();
    auto end = clock();
    time = 1000.0 * (end - start) / CLOCKS_PER_SEC;
    if (true)
    {   // Tiene salida
      cout << "Shortest path length = " << solution << endl <<
              "Explored nodes= " << explored << " (Added= " << added <<
              "; nonpromising= " << no_promising << "; nonfactible= " << no_feasible << ')' << endl <<
              "Promising but discarded nodes =" << prom_but_disc << endl <<
              "Best solution update from a completed node=" << update_solution << endl <<
              "Best solution update from a pessimistic bound= " << update_solution << endl <<
              "CPU lapsed time=" << time << " ms." << endl;
    }
  }
  else
  {
    cout << "Error" << endl;
  }

  return 0;
}