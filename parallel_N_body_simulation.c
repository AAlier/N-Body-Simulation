#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <mpi.h>
#define MATH_PI 3.14159265358979323846

typedef struct _body {
	float x, y;	// position
	float ax, ay;	// acceleration
	float vx, vy;	// velocity
	float mass;	// mass
} body;

void integrate(body *body, float deltaTime) {
        body->vx += body->ax * deltaTime;
        body->vy += body->ay * deltaTime;
        body->x += body->vx * deltaTime;
        body->y += body->vy * deltaTime;
}

void calculateNewtonGravityAcceleration(body *a, body *b, float *ax, float *ay) {
	float softening = 10000;
	float distanceX = b->x - a->x;
        float distanceY = b->y - a->y;
        float vectorDistance = a->x * a->x + a->y * a->y + softening;
        float vectorDistanceCubed = vectorDistance * vectorDistance * vectorDistance;
        float inverse = 1.0 / sqrt(vectorDistanceCubed);
        float scale = b->mass * inverse;
        *ax = (distanceX * scale);
        *ay = (distanceY * scale);
}

void simulateWithBruteforce(int rank, int totalBodies, int nBodies, body *bodies, body *local_bodies, float dt) {
        for(size_t i = 0; i < nBodies; i++) {
                float total_ax = 0, total_ay = 0;
                for (size_t j = 0; j < totalBodies; j++) {
                        if (i == nBodies * rank + i) {
                                continue;
                        }
                        float ax, ay;
                        calculateNewtonGravityAcceleration(&local_bodies[i], &bodies[j], &ax, &ay);
                        total_ax += ax;
                        total_ay += ay;
                }
                local_bodies[i].ax = total_ax;
                local_bodies[i].ay = total_ay;
                integrate(&local_bodies[i], dt);
        }
}

float randValue(){
        return ((float) rand() / RAND_MAX);
}

body *initBodies (int nBodies) {
        srand(time(NULL));
        const float accelerationScale = 100.0;
        body *bodies = (body *) malloc(sizeof(*bodies) * nBodies);
        for (int i = 0; i < nBodies; i++) {
                float angle = ((float) i / nBodies) * 2.0 * MATH_PI +
                                                ((randValue() - 0.5) * 0.5);
                float initialMass = 2;
                body object = {
                        .x = randValue(), .y = randValue(),
                        .vx = cos(angle) * accelerationScale * randValue(),
                        .vy = sin(angle) * accelerationScale * randValue(),
                        .mass = randValue() + initialMass * 0.5
                };
        bodies[i] = object;
    }
    return bodies;
}

int main(int argc, char **argv) {
	float delta_time = 0.01;
	int nBodies = 100;
	float simulation_time = 1.0f;
	if (argc > 3) {
		delta_time = atof(argv[1]);
		nBodies = atoi(argv[2]);
		simulation_time = atof(argv[3]);
	}
	double parallel_average_time = 0.0;
	MPI_Init(&argc, &argv);
	int world_size, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Datatype dt_body;
	MPI_Type_contiguous(7, MPI_FLOAT, &dt_body);
	MPI_Type_commit(&dt_body);

	if (rank == 0) {
		parallel_average_time -= MPI_Wtime();
	}
    	size_t items_per_process = nBodies / world_size;
    	body *bodies = initBodies(nBodies);
    	if (rank == 0) {
    	    bodies = initBodies(nBodies);
    	}
	body *local_bodies = (body *) malloc(sizeof(*local_bodies) * items_per_process);


    	MPI_Scatter(
        	bodies,
        	items_per_process,
        	dt_body,
        	local_bodies,
       	 	items_per_process,
		dt_body,
       	 	0,
    	    	MPI_COMM_WORLD
    	);
	simulateWithBruteforce(rank, nBodies, items_per_process, bodies, local_bodies, delta_time);

	body *gathered_bodies = NULL;
	if(rank == 0){
		gathered_bodies = (body*) malloc(sizeof(*gathered_bodies) * nBodies);
	}
	MPI_Gather(
	        local_bodies,
	        items_per_process,
        	dt_body,
        	bodies,
        	items_per_process,
        	dt_body,
        	0,
        	MPI_COMM_WORLD
    	);
	if (rank == 0) {
        	parallel_average_time += MPI_Wtime();
		printf("%d\n%.5f\n%.5f\n", nBodies, simulation_time, delta_time);
        	for (size_t i = 0; i < nBodies; ++i){
			printf("%.5f, %.5f\n", bodies[i].x, bodies[i].y);
			printf("%.5f, %.5f\n", bodies[i].ax, bodies[i].ay);
			printf("%.5f, %.5f\n, %.5f\n", bodies[i].vx, bodies[i].vy, bodies[i].mass);
        	}
		for(float j = 0; j < simulation_time; j += delta_time){
			for(size_t i = 0; i < nBodies; ++i){
				printf("%.5f, %.5f\n", gathered_bodies[i].ax, gathered_bodies[i].ay);
				integrate(&gathered_bodies[i], delta_time);
			}
		}
    	}
	if (bodies != NULL) {
        	free(bodies);
	}
    free(local_bodies);
	if(gathered_bodies != NULL){
		free(gathered_bodies);
	}
    MPI_Finalize();
	return 0;
}
