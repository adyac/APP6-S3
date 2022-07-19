#include "robotdiag.hpp"
#include "robotsim.hpp"

#include <vector>
#include <queue>
#include <cstdio>
#include <thread>
#include <mutex>
#include <condition_variable>

//Variables partagées: queue_, data_ 


std::mutex mutex_;
std::condition_variable queue_cond;
std::condition_variable data_cond;

using namespace s3gro;

RobotDiag::RobotDiag(){}

// Le destructeur sera normalement appellé à la fermeture de l'application.
// Écrit des statistiques à l'écran.
RobotDiag::~RobotDiag() {
    stop_recording();
}

void RobotDiag::push_event(RobotState new_robot_state) {
    // Conserve toutes les données dans data_
    data_.push_back(new_robot_state);
    
    data_cond.notify_one(); 	//When previous line has been executed (data_.push_back: data has been added), 
				//we can then satisfy the condition_variable wait in start_recording().
    

    // Ajoute le dernier événement à la file d'exportation queue_. 
    //On aura besoin de synchroniser queue_ avec export_loop (pour le CSV) et non avec start/stop_recording
    
    queue_.push(new_robot_state); 
    data_cond.notify_one();
     
}

void RobotDiag::set_csv_filename(const std::string& file_name) {
    csv_filename_ = file_name;
}

void RobotDiag::start_recording() {
    // Indique que le système de diagnostic fonctionne (à mettre à 'false' lors
    // de la fermeture pour interrompre le fil d'exportation).

    run_ = true;

    // Démarre le simulateur:
    // TODO: Supprimer cette ligne si vous testez avec un seul moteur
    // robotsim::init(this, 8, 10, 3);  // Spécifie le nombre de moteurs à 
                                        // simuler (8) et le délai moyen entre
                                        // les événements (10 ms) plus ou moins
                                        // un nombre aléatoire (3 ms)
    // TODO : Lancement du fil.
    
    
    std::thread Thread_Record; //On start un thread, mais quel est l'état initial du thread
   
   
    std::unique_lock<std::mutex> lock(mutex_); // Utiliser unique_lock, car on a un condition_variable. 
					       // Lock_guard ferme le thread jusqu'a sa destruction, cest pourquoi on l'utilise pas.
    
    
    data_cond.wait(lock, []{return !data_.empty();}); // Thread_1 attend que le thread principal ait stocké les informations dans data_.
						     // Lorsque notify_one a été exécuté, on peut enlever condition_variable si data_ != vide
    
    //Faire fonctions pour copier les elements de data?
    
    
    
}

void RobotDiag::stop_recording() {
    // Indique que le système de diagnostic doit être arrêté.
    run_ = false;

    // TODO : Fermeture du fil.
    
    
    Thread_Record.join(); 
    
    
    robotsim::stop_and_join();

    printf("Final vector size: %zu\n", data_.size());
}




// Fonction d'exportation vers CSV.
// Doit être exécutée dans un fil séparé et écrire dans le fichier CSV
// lorsque de nouvelles données sont disponibles dans queue_.
void RobotDiag::export_loop() {
    
    
    
    std::thread Thread_CSV; // Quel devrait être l'état initial de ce fil?
    
    
    
    if (csv_filename_.empty()) {
        csv_filename_ = "/tmp/robotdiag.csv";
    }

    FILE* out = fopen(csv_filename_.c_str(), "w");

    if (out == NULL) {
        printf("ERROR: Cannot open output file.\n");
        return;
    }

    // En-tête du fichier CSV, respectez le format.
    fprintf(out, "motor_id;t;pos;vel;cmd\n");
    
   
    // TODO: Synchronisation et écriture.
    
    //ATTENTION queue_ et data_ sont privées, mais on a besoin de les acceder pour les écrire dans le CSV
    
    /*
    std::unique_lock<std::mutex> lock(mutex_);
    queue_cond.wait(lock, []{return !queue_.empty()});
    
    for ()
    {
    	fprintf(out, queue_.front());
    	queue_.pop();
    
    }
    
    */
    fclose(out);
}

