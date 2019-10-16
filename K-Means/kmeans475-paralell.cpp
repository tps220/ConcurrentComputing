// Implementation of the KMeans Algorithm
// reference: http://mnemstudio.org/clustering-k-means-example-1.htm

#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <chrono>
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"
#include "tbb/partitioner.h"
#include <pthread.h>
using namespace std;

#ifdef USE_SPINLOCK
	#define LOCK_TYPE pthread_spinlock_t
	#define LOCK(x) pthread_spin_lock(&(x))
	#define UNLOCK(x) pthread_spin_unlock(&(x))
	#define INITIALIZE(x) pthread_spin_init(&(x), NULL)
#else
	#define LOCK_TYPE pthread_mutex_t
	#define LOCK(x) pthread_mutex_lock(&(x))
	#define UNLOCK(x) pthread_mutex_unlock(&(x))
	#define INITIALIZE(x) pthread_mutex_init(&(x), NULL)
#endif


static unsigned long a = 123456789, b = 362436069, c = 521288629;
unsigned long xorshf96() {
	unsigned long t;
  a ^= a << 16;
  a ^= a >> 5;
  a ^= a << 1;

  t = a;
  a = b;
  b = c;
  c = t ^ a ^ b;
  return c;
}

class Point {
private:
	int id_point, id_cluster;
	vector<double> values;
	int total_values;
	string name;

public:
	Point(int id_point, vector<double>& values, string name = "") {
		this->id_point = id_point;
		total_values = values.size();

		for (int i = 0; i < total_values; i++) {
			this->values.push_back(values[i]);
		}

		this->name = name;
		id_cluster = -1;
	}

	int getID() {
		return id_point;
	}

	void setCluster(int id_cluster) {
		this->id_cluster = id_cluster;
	}

	int getCluster() {
		return id_cluster;
	}

	double getValue(int index) {
		return values[index];
	}

	int getTotalValues() {
		return total_values;
	}

	void addValue(double value) {
		values.push_back(value);
	}

	string getName() {
		return name;
	}
};

class Cluster {
private:
	int id_cluster;
	vector<double> central_values;
	vector<Point> points;

public:
	Cluster(int id_cluster, Point point) {
		this->id_cluster = id_cluster;
		int total_values = point.getTotalValues();

		for (int i = 0; i < total_values; i++) {
			central_values.push_back(point.getValue(i));
		}

		points.push_back(point);
	}

	void addPoint(Point point) {
		points.push_back(point);
	}

	void safeAddPoint(Point point, LOCK_TYPE *tsx_mtx) {
		LOCK(*tsx_mtx);
		addPoint(point);
		UNLOCK(*tsx_mtx);
	}

	void safeRemovePoint(int id_point, LOCK_TYPE *tsx_mtx) {
		LOCK(*tsx_mtx);
		removePoint(id_point);
		UNLOCK(*tsx_mtx);
	}

	bool removePoint(int id_point) {
		int total_points = points.size();

		for (int i = 0; i < total_points; i++) {
			if (points[i].getID() == id_point) {
				points.erase(points.begin() + i);
				return true;
			}
		}
		return false;
	}

	double getCentralValue(int index) {
		return central_values[index];
	}

	void setCentralValue(int index, double value) {
		central_values[index] = value;
	}

	Point getPoint(int index) {
		return points[index];
	}

	int getTotalPoints() {
		return points.size();
	}

	int getID() {
		return id_cluster;
	}
};

class KMeans {
private:
	int K; // number of clusters
	int total_values, total_points, max_iterations;
	vector<Cluster> clusters;
	LOCK_TYPE* tsx_mtx;

	// return ID of nearest center (uses euclidean distance)
	int getIDNearestCenter(Point point) {
		double sum = 0.0, min_dist;
		int id_cluster_center = 0;

		#pragma omp simd
		for (int i = 0; i < total_values; i++) {
			int val = clusters[0].getCentralValue(i) - point.getValue(i);
			sum += val * val;
		}

		min_dist = sqrt(sum);

		for (int i = 1; i < K; i++) {
			double dist;
			sum = 0.0;

			#pragma omp simd
			for (int j = 0; j < total_values; j++) {
				int val = clusters[i].getCentralValue(j) - point.getValue(j);
				sum += val * val;
			}

			dist = sqrt(sum);

			if (dist < min_dist) {
				min_dist = dist;
				id_cluster_center = i;
			}
		}
		return id_cluster_center;
	}

public:
	KMeans(int K, int total_points, int total_values, int max_iterations) {
		this->K = K;
		this->total_points = total_points;
		this->total_values = total_values;
		this->max_iterations = max_iterations;

		this -> tsx_mtx = new LOCK_TYPE[K];
		for (int i = 0; i < K; i++) {
			INITIALIZE(this -> tsx_mtx[i]);
		}
	}

	~KMeans() {
		delete [] tsx_mtx;
	}

	void run(vector<Point>& points) {
    auto begin = chrono::high_resolution_clock::now();
        
		if (K > total_points) {
			return;
		}
		//Assign K random indexes to become cluster centers
		vector<int> prohibited_indexes;
		for (int i = 0; i < K; i++) {
			int index_point = xorshf96() % total_points;
			if (find(prohibited_indexes.begin(), prohibited_indexes.end(), index_point) == prohibited_indexes.end()) {
				prohibited_indexes.push_back(index_point);
				points[index_point].setCluster(i);
				Cluster cluster(i, points[index_point]);
				clusters.push_back(cluster);
			}
			else {
				i--;
			}
		}
    auto end_phase1 = chrono::high_resolution_clock::now();
    //end

		//Group elements into perfect clusters or execute x iterations to approximate clusters
		int iter = 1;
		while(true) {
			bool done = true;
			// associates each point to the nearest center
			tbb::parallel_for(tbb::blocked_range<size_t>(0, total_points), [&](tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); i++) {
					int id_old_cluster = points[i].getCluster();
					int id_nearest_center = getIDNearestCenter(points[i]);

					if (id_old_cluster != id_nearest_center) {
						if (id_old_cluster != -1) {
							clusters[id_old_cluster].safeRemovePoint(points[i].getID(), &this -> tsx_mtx[id_old_cluster]); //TODO: Wrap in a synchronzied block
						}

						points[i].setCluster(id_nearest_center);
						clusters[id_nearest_center].safeAddPoint(points[i], &this -> tsx_mtx[id_nearest_center]);  //TODO: Wrap in a synchronzied block
						done = false;
					}
				}
			}, tbb::simple_partitioner());

			//recalculating the center of each cluster
			for (int i = 0; i < K; i++) {
				for (int j = 0; j < total_values; j++) {
					int total_points_cluster = clusters[i].getTotalPoints();
					double sum = 0.0;
					if (total_points_cluster > 0) {
						int sum = tbb::parallel_reduce(tbb::blocked_range<int>(0, total_points_cluster), 0, [&](tbb::blocked_range<int> r, int running_total) {
  						for (int p = r.begin(); p < r.end(); p++) {
    						running_total += clusters[i].getPoint(p).getValue(j);
    					}
    					return running_total;
						}, std::plus<int>());
						clusters[i].setCentralValue(j, sum / total_points_cluster);
					}
				}
			}

			if (done == true || iter >= max_iterations) {
				cout << "Break in iteration " << iter << "\n\n";
				break;
			}
			iter++;
		}
    auto end = chrono::high_resolution_clock::now();

		// shows elements of clusters
		/* for (int i = 0; i < K; i++) {
			int total_points_cluster =  clusters[i].getTotalPoints();

			cout << "Cluster " << clusters[i].getID() + 1 << endl;
			for (int j = 0; j < total_points_cluster; j++) {
				cout << "Point " << clusters[i].getPoint(j).getID() + 1 << ": ";
				for (int p = 0; p < total_values; p++) {
					cout << clusters[i].getPoint(j).getValue(p) << " ";
				}

				string point_name = clusters[i].getPoint(j).getName();

				if (point_name != "") {
					cout << "- " << point_name;
				}
				cout << endl;
			}

			cout << "Cluster values: ";

			for (int j = 0; j < total_values; j++) {
				cout << clusters[i].getCentralValue(j) << " ";
			}
		}
		cout << "\n\n";
		*/
    cout << "TOTAL EXECUTION TIME = "<<std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count()<<"\n";
 		cout << "TIME PHASE 1 = "<<std::chrono::duration_cast<std::chrono::microseconds>(end_phase1-begin).count()<<"\n";
		cout << "TIME PHASE 2 = "<<std::chrono::duration_cast<std::chrono::microseconds>(end-end_phase1).count()<<"\n";
	}
};

int main(int argc, char *argv[]) {
	srand (time(NULL));
	int total_points, total_values, K, max_iterations, has_name;
	cin >> total_points >> total_values >> K >> max_iterations >> has_name;

	vector<Point> points;
	string point_name;

	for (int i = 0; i < total_points; i++) {
		vector<double> values;

		for (int j = 0; j < total_values; j++) {
			double value;
			cin >> value;
			values.push_back(value);
		}

		if (has_name) {
			cin >> point_name;
			Point p(i, values, point_name);
			points.push_back(p);
		}
		else {
			Point p(i, values);
			points.push_back(p);
		}
	}
	KMeans kmeans(K, total_points, total_values, max_iterations);
	kmeans.run(points);

	return 0;
}
