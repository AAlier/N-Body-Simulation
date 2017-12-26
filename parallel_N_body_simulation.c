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
        body->x += body->vx * deltaTime + (1 / 2) * body->ax * deltaTime * deltaTime;
        body->y += body->vy * deltaTime + (1 / 2) * body->ay * deltaTime * deltaTime;
        body->vx += body->ax * deltaTime;
        body->vy += body->ay * deltaTime;
}

void calculateNewtonGravityAcceleration(body *a, body *b, float *ax, float *ay) {
        float distanceX = fabsf(b->x - a->x);
        float distanceY = fabsf(b->y - a->y);
        float vectorDistance = sqrt(a->x * a->x + a->y * a->y);
        float vectorDistanceCubed = vectorDistance * vectorDistance * vectorDistance;
        float inverse = 1.0 / vectorDistanceCubed;
        float scale = b->mass * inverse;
        *ax = (distanceX * scale);
        *ay = (distanceY * scale);
}

void simulateWithBruteforce(int nBodies, body *bodies, float dt) {
        for(size_t i = 0; i < nBodies; i++) {
                float total_ax = 0, total_ay = 0;
                for (size_t j = 0; j < nBodies; j++) {
                        if (i == j) {
                                continue;
                        }
                        float ax, ay;
                        calculateNewtonGravityAcceleration(&bodies[i], &bodies[j], &ax, &ay);
                        total_ax += ax;
                        total_ay += ay;
                }
                bodies[i].ax = total_ax;
                bodies[i].ay = total_ay;
                integrate(&bodies[i], dt);
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
	if (argc > 2) {
		delta_time = atof(argv[1]);
		nBodies = atoi(argv[2]);
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
    body *bodies = NULL;
    bodies = initBodies(nBodies);
    if (rank == 0) {
        bodies = initBodies(nBodies);
    }
	body *local_bodies =
        (body *) malloc(sizeof(*local_bodies) * items_per_process);


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
	simulateWithBruteforce(items_per_process, local_bodies, delta_time);
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
        printf("Number of bodies: %.2d, time: %.2f\n", nBodies, parallel_average_time);
        for (size_t i = 0; i < nBodies; ++i){
		printf("Body #: %d, accelerationX - %f, accelerationY - %f\n", (int)i, bodies[i].ax, bodies[i].ay);
        }
    }
	if (bodies != NULL) {
        free(bodies);
    }
    free(local_bodies);
    MPI_Finalize();
	return 0;
}
