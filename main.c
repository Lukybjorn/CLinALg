#include <complex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Print the error messege in the <fmt> specified and exit the program with EXIT_FALIURE
#define raiseError(fmt, ...) do { \
    fprintf(stderr, "ERROR[%d:%s()] ", __LINE__, __func__); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(EXIT_FAILURE); \
} while (false)

// Print a warning messege in the <fmt> specified and continues the program
#define raiseWarning(fmt, ...) do { \
    fprintf(stderr, "WARNING[%d:%s()] ", __LINE__, __func__); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
} while (false)

// Define a struct Matrix for complex valued matrices
typedef struct Matrix {
    complex double *data;
    int rows;
    int cols;
} Matrix;


/*
 * Convert a string to a complex double.
 *
 * Supported formats:
 *
 *     3.5
 *     3.5I
 *     3.5+2.7I
 *     3.5-2.7I
 *     -3.5+2.7I
 *     2.7e-3I
 *
 * endptr is set to the first character that was not
 * part of the complex number.
 */
double complex strtocd(const char *str, char **endptr)
{
    double real = 0.0;
    double imag = 0.0;

    /* Read first number */
    double value = strtod(str, endptr);

    /* Check whether strtod actually found a number */
    if (*endptr == str) {
        return 0.0 + 0.0 * I;
    }

    /*
     * First number is imaginary:
     *
     *     3.5I
     */
    if (**endptr == 'I' || **endptr == 'i') {
        imag = value;
        (*endptr)++;

        return imag * I;
    }

    /*
     * First number is real:
     *
     *     3.5
     *     3.5+2.7I
     *     3.5-2.7I
     */
    real = value;

    /*
     * Check whether there is an imaginary part.
     */
    if (**endptr == '+' || **endptr == '-') {

        value = strtod(*endptr, endptr);

        if (*endptr != NULL &&
            (**endptr == 'I' || **endptr == 'i')) {

            imag = value;
            (*endptr)++;
        }
    }

    return real + imag * I;
}


/*
 * Read a matrix from a file.
 *
 * Each row is one line.
 * Columns are separated by sep.
 */
Matrix read_matrix(const char *filename, char sep)
{
    Matrix matrix = {NULL, 0, 0};

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        raiseError("raiseError opening file '%s'", filename);
        return matrix;
    }


    /*
     * --------------------------------------------------
     * First pass: determine dimensions
     * --------------------------------------------------
     */

    char *line = NULL;
    size_t len = 0;

    int n_cols = -1;

    while (getline(&line, &len, file) != -1) {

        /* Skip empty lines */
        if (line[0] == '\n' || line[0] == '\0')
            continue;


        /*
         * Count rows.
         */
        matrix.rows++;


        /*
         * Determine number of columns from
         * the first non-empty row.
         */
        if (n_cols == -1) {

            matrix.cols = 1;

            for (int i = 0; line[i] != '\0'; i++) {

                if (line[i] == sep)
                    matrix.cols++;
            }

            n_cols = matrix.cols;
        }


        /*
         * Check that subsequent rows have the
         * same number of columns.
         */
        else {

            int counter = 1;

            for (int i = 0; line[i] != '\0'; i++) {

                if (line[i] == sep)
                    counter++;
            }

            if (counter != n_cols) {

                raiseError(
                    "Inconsistent number of columns "
                    "in row %d: expected %d, found %d",
                    matrix.rows,
                    n_cols,
                    counter
                );

                free(line);
                fclose(file);

                matrix.rows = 0;
                matrix.cols = 0;

                return matrix;
            }
        }
    }


    free(line);
    line = NULL;
    len = 0;


    /*
     * Check for an empty file.
     */
    if (matrix.rows == 0 || matrix.cols == 0) {

        fclose(file);

        raiseError("Empty matrix");

        return matrix;
    }


    /*
     * --------------------------------------------------
     * Allocate memory
     * --------------------------------------------------
     */

    matrix.data = malloc(
        matrix.rows *
        matrix.cols *
        sizeof(*matrix.data)
    );


    if (matrix.data == NULL) {

        raiseError("Memory allocation failed");

        fclose(file);

        matrix.rows = 0;
        matrix.cols = 0;

        return matrix;
    }


    /*
     * --------------------------------------------------
     * Second pass: read values
     * --------------------------------------------------
     */

    rewind(file);

    int row = 0;

    while (getline(&line, &len, file) != -1) {

        /* Skip empty lines */
        if (line[0] == '\n' || line[0] == '\0')
            continue;


        char *ptr = line;


        for (int col = 0; col < matrix.cols; col++) {

            char *endptr;


            /*
             * Read complex number.
             */
            matrix.data[
                row * matrix.cols + col
            ] = strtocd(ptr, &endptr);


            /*
             * Check that something was actually parsed.
             */
            if (endptr == ptr) {

                raiseError(
                    "Invalid number at row %d, column %d",
                    row + 1,
                    col + 1
                );

                free(line);
                free(matrix.data);
                fclose(file);

                matrix.data = NULL;
                matrix.rows = 0;
                matrix.cols = 0;

                return matrix;
            }


            /*
             * Move to the next column.
             */
            if (col < matrix.cols - 1) {

                /*
                 * Find separator.
                 */
                while (
                    *endptr != sep &&
                    *endptr != '\0' &&
                    *endptr != '\n'
                ) {
                    endptr++;
                }


                /*
                 * We must find the separator.
                 */
                if (*endptr != sep) {

                    raiseError(
                        "Expected separator at row %d, "
                        "column %d",
                        row + 1,
                        col + 1
                    );

                    free(line);
                    free(matrix.data);
                    fclose(file);

                    matrix.data = NULL;
                    matrix.rows = 0;
                    matrix.cols = 0;

                    return matrix;
                }


                /*
                 * Skip separator.
                 */
                ptr = endptr + 1;
            }
        }


        row++;
    }


    free(line);
    fclose(file);

    return matrix;
}

void show(Matrix *mat){
    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
            complex double z = mat->data[i * mat->cols + j];
            printf("%8.3f%+8.3fi", creal(z), cimag(z));
            if (j < mat->cols - 1) printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}

int add_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out){
    if (mat_1->rows != mat_2->rows || mat_1->cols != mat_2->cols){
        raiseError("Incorrect shapes for %s operation", "subtraction");
        return 1;
    }
    int length = mat_1->rows * mat_1->cols;
    for (int i = 0; i < length ; i++){
        mat_out->data[i] = mat_1->data[i] + mat_2->data[i];
    }
    return 0;
}

int sub_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out){
    if (mat_1->rows != mat_2->rows || mat_1->cols != mat_2->cols){
        raiseError("Incorrect shapes for %s operation", "addition");
        return 1;
    }
    int length = mat_1->rows * mat_1->cols;
    for (int i = 0; i < length ; i++){
        mat_out->data[i] = mat_1->data[i] - mat_2->data[i];
    }
    return 0;
}

int mul_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out)
{
    if (mat_1->cols != mat_2->rows) {
        raiseError("Incorrect shapes for %s operation", "multiplication");
        return 1;
    }

    int rows = mat_1->rows;
    int cols = mat_2->cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {

            mat_out->data[i * cols + j] = 0;

            for (int k = 0; k < mat_1->cols; k++) {
                mat_out->data[i * cols + j] += mat_1->data[i * mat_1->cols + k] * mat_2->data[k * mat_2->cols + j];
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: %s <filename_1> <filename_2> <operation>\n", argv[0]);
        printf("operation: add, sub, mul\n"); //TODO: add more operations
        return 1;
    }

    Matrix mat_1 = read_matrix(argv[1], ',');
    Matrix mat_2 = read_matrix(argv[2], ',');

    if (mat_1.data == NULL || mat_2.data == NULL) {
        raiseError("Invalid matrices supplied");
        return 1;
    }

    show(&mat_1);
    show(&mat_2);

    Matrix mat_out;
    mat_out.rows = mat_1.rows;
    mat_out.cols = mat_1.cols;
    mat_out.data = malloc(sizeof(complex double) * mat_out.rows * mat_out.cols); // allocate memory for the output matrix

    printf("Addition\n");
    add_matrix(&mat_1, &mat_2, &mat_out);
    show(&mat_out);

    printf("Subtruction\n");
    sub_matrix(&mat_1, &mat_2, &mat_out);
    show(&mat_out);

    printf("Multipication\n");
    mul_matrix(&mat_1, &mat_2, &mat_out);
    show(&mat_out);

    free(mat_1.data);
    free(mat_2.data);
    free(mat_out.data);


    return 0;
}
