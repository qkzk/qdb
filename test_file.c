#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char* nom;
  int age;
  char* adresse;
} Personne;

int write_p() {
  /* Personne p = {"Jean Dupont", 35, "1 rue de la Paix"}; */
  Personne* p = (Personne*)malloc(sizeof(Personne));
  assert(p != NULL);
  p->age = 35;
  p->nom = (char*)malloc(sizeof(char) * 100);
  assert(p->nom != NULL);
  strcpy(p->nom, "Jean Dupont");
  p->adresse = (char*)malloc(sizeof(char) * 100);
  assert(p->adresse != NULL);
  strcpy(p->adresse, "1 rue de la paix");

  FILE* fichier = fopen("data.bin", "wb");
  if (fichier == NULL) {
    printf("Erreur d'ouverture du fichier\n");
    return 1;
  }

  fwrite(p, sizeof(Personne), 1, fichier);

  fclose(fichier);

  return 0;
}

int read_p() {
  Personne p;

  FILE* fichier = fopen("data.bin", "rb");
  if (fichier == NULL) {
    printf("Erreur d'ouverture du fichier\n");
    return 1;
  }

  fread(&p, sizeof(Personne), 1, fichier);

  printf("Nom: %s\n", p.nom);
  printf("Age: %d\n", p.age);
  printf("Adresse: %s\n", p.adresse);

  fclose(fichier);

  return 0;
}

int main(void) {
  write_p();
  read_p();
  printf("done\n");

  return 0;
}
