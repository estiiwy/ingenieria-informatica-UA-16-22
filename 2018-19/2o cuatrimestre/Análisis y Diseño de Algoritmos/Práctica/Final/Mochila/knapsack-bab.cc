// MOCHILA DISCRETA - BRANCH AND BOUND

#include <iostream>
#include <vector>
#include <queue>

using namespace std;


float knapsack(const vector<double> &v, const vector<double> &w, double W)
{
	typedef vector<short> sol;
	typedef tuple<double, double, sol, int> node;	// value, weight, vector, k
	priority_queue<node> pq;

	double best_val = knapsack_d(v, w, 0, W);
	pq.push(node(0.0, 0.0, sol(v.size()), 0));

	while (!pq.empty())
	{
		double value, weight;
		sol x;
		unsigned k;

		tie(value, weight, x, k) = pq.top();
		pq.top();

		if (k == v.size())							// base case
		{
			best_val = max(value, best_val);
			continue;
		}

		for (unsigned j = 0; j < 2; j++)			// no base case
		{	// Hace combinación con no coger y coger (0 o 1)
			x[k] = j;

			double new_weight = weight + x[k] * w[k];	// updating weight
			double new_value = value + x[k] * w[k];		// updating value

			if (new_weight <= W &&						// is feasible & is promising
				new_value + knapsack_c(v, w, k + 1, W - new_weight) > best_val)
			{
				pq.push(node(new_value, new_weight, x, k + 1));
			}
		}
	}

	return best_val;
}

int main()
{
	vector<double> values = (49, 40, 20);
	vector<double> weights = ();

	return 0;
}