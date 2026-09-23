#include <complex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    size_t rows;
    size_t cols;
} Matrix;

/*
 * Initilize a matrix element in size (rows, cols) filled with <fill>
 */
Matrix init_matrix(size_t rows, size_t cols, complex double fill) {
    Matrix mat = {NULL, rows, cols};
    mat.data = calloc(rows * cols, sizeof(fill));
    if (mat.data == NULL) {
        raiseError("Memory allocation failed");
    }
    if (fill != 0.0) {
        size_t length = rows * cols;
        for (int i = 0; i < length; i++) {
            mat.data[i] = fill;
        }
    } 
    return mat;
}

/*
 * Convert a string to a complex double.
 *
 * Supported formats (I or i):
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

    // Read first number
    char *p;
    double value = strtod(str, &p);

    // strtod found no number, Raise error and exit
    if (p == str) {
        raiseError("Input string [ %s ] could not be converted into a double.", str);
    }

    // First number is imaginary
    if (*p == 'I' || *p == 'i') {
        imag = value;
        p++;

        if (endptr != NULL) *endptr = p;

        return imag * I;
    }

    // First number is real
    real = value;

    // Check whether there is an imaginary part
    if (*p == '+' || *p == '-') {

        char *imag_end;
        double imag_value = strtod(p, &imag_end);

        // No imaginary number was found, Raise error and exit
        if (imag_end == p) {
            raiseError("Expected an imaginary part after [ %.*s ].", (int)(p - str + 1), str);
        }

        // Number must be followed by i/I 
        if (*imag_end == 'I' || *imag_end == 'i') {
            imag = imag_value;
            p = imag_end + 1;
        } else { //Followed by a different character, Raise error and exit
            raiseError("Imaginary part [ %.*s ] is not followed by 'I' or 'i'. Returning [ 0+0i ]", (int)(imag_end - p), p);
        }
    }

    if (endptr != NULL) *endptr = p;

    return real + imag * I;
}

/*
 * Read a matrix from a file.
 *
 * Each row is one line.
 * Columns are separated by <sep>.
 */
Matrix read_matrix(const char *filename, char sep) {
    Matrix matrix = {NULL, 0, 0};

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        raiseError("raiseError opening file '%s'", filename);
    }

    // Determine dimensions
    char *line = NULL;
    size_t len = 0;

    int n_cols = -1;

    while (getline(&line, &len, file) != -1) {

        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;

        // Count rows.
        matrix.rows++;

        // Determine number of columns from the first non-empty row.
        if (n_cols == -1) {

            matrix.cols = 1;

            for (int i = 0; line[i] != '\0'; i++) {
                if (line[i] == sep && line[i+1] != '\n') matrix.cols++;
                
            }

            n_cols = matrix.cols;
        }

        // Check that subsequent rows have the same number of columns.
        else {
            int counter = 1;

            for (int i = 0; line[i] != '\0'; i++) {
                if (line[i] == sep && line[i+1] != '\n') counter++;
            }

            if (counter != n_cols) {
                fclose(file);
                raiseError("Inconsistent number of columns in row %d: expected %d, found %d", (int)(matrix.rows), n_cols, counter);
            }
        }
    }

    free(line);
    line = NULL;
    len = 0;


    
    // Check for an empty file.
    if (matrix.rows == 0 || matrix.cols == 0) {
        fclose(file);
        raiseError("Empty matrix");
    }


    // Allocate memory for the matrix
    matrix.data = malloc(matrix.rows * matrix.cols * sizeof(*matrix.data));

    if (matrix.data == NULL) {
        fclose(file);
        raiseError("Memory allocation failed");
    }
    
    // Read values into the <data> array row by row
    rewind(file);

    int row = 0;

    while (getline(&line, &len, file) != -1) {

        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;

        char *ptr = line;

        for (int col = 0; col < matrix.cols; col++) {
            char *endptr;

            // Read complex number and store in the matrix.data variable row wise
            matrix.data[row * matrix.cols + col] = strtocd(ptr, &endptr);
        //    printf("%f%+f\n", creal(matrix.data[row * matrix.cols + col]), cimag(matrix.data[row * matrix.cols + col]));

            // Check that something was converted succsesfuly to complex double
            if (endptr == ptr) {
                free(line);
                free(matrix.data);
                fclose(file);
                raiseError("Invalid number at row %d, column %d", row + 1, col + 1);
            }

            // Move to the next item in the row
            if (col < matrix.cols - 1) {

                // Find separator
                while (*endptr != sep && *endptr != '\0' && *endptr != '\n') {
                    endptr++;
                }

                // If no separator found, crash
                if (*endptr != sep) {
                    free(line);
                    free(matrix.data);
                    fclose(file);
                    raiseError("Expected separator at row %d, column %d", row + 1, col + 1);
                }

                // Seperator found, going to the next item in the row
                ptr = endptr + 1;
            }
        }

        row++;
    }

    free(line);
    fclose(file);

    return matrix;
}

/*
 * Print a matrix with complex values 
 */
void show(Matrix *mat) {
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

/*
 * Add two matrices with complex values. Must be the same dimentions
 */
void add_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out) {
    // If the dimentions does not match
    if (mat_1->rows != mat_2->rows || mat_1->cols != mat_2->cols) raiseError("Incorrect shapes for %s operation", "addition");

    int length = mat_1->rows * mat_1->cols;
    for (int i = 0; i < length ; i++) {
        mat_out->data[i] = mat_1->data[i] + mat_2->data[i];
    }
}

/*
 * Subtract two matrices with complex values. Must be the same dimentions
 */
void sub_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out) {
    // If the dimentions does not match
    if (mat_1->rows != mat_2->rows || mat_1->cols != mat_2->cols) raiseError("Incorrect shapes for %s operation", "addition");

    int length = mat_1->rows * mat_1->cols;
    for (int i = 0; i < length ; i++) {
        mat_out->data[i] = mat_1->data[i] - mat_2->data[i];
    }
}

/*
 * Multiply two matrices with complex values.
 * mat_1.cols must equal mat_2.rows (mXn * nXk) -> (mXk)
 */
void mul_matrix(Matrix *mat_1, Matrix *mat_2, Matrix *mat_out) {
    // If the dimentions does not match
    if (mat_1->cols != mat_2->rows) raiseError("Incorrect shapes for %s operation", "multiplication");

    size_t rows = mat_1->rows;
    size_t cols = mat_2->cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
        
            // Initilize each cell in mat_out to 0
            mat_out->data[i * cols + j] = 0;

            for (int k = 0; k < mat_1->cols; k++) {
                mat_out->data[i * cols + j] += mat_1->data[i * mat_1->cols + k] * mat_2->data[k * mat_2->cols + j];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    const char *operations = "add, sub, mul";

    if (argc < 4) {
        raiseError("Usage: %s <filename_1> <filename_2> <operation>\nValid operations: %s\n", argv[0], operations);
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

    if (strcmp(argv[3], "add") == 0) {

        printf("Addition\n");

        mat_out = init_matrix(mat_1.rows, mat_1.cols, 0);

        add_matrix(&mat_1, &mat_2, &mat_out);
        show(&mat_out);

    } else if (strcmp(argv[3], "sub") == 0) {

        printf("Subtraction\n");

        mat_out = init_matrix(mat_1.rows, mat_1.cols, 0);

        sub_matrix(&mat_1, &mat_2, &mat_out);
        show(&mat_out);

    } else if (strcmp(argv[3], "mul") == 0) {

        printf("Multiplication\n");

        mat_out = init_matrix(mat_1.rows, mat_2.cols, 0);

        mul_matrix(&mat_1, &mat_2, &mat_out);
        show(&mat_out);

    } else {
        raiseError("Valid operations: %s", operations);
    }

    free(mat_1.data);
    free(mat_2.data);
    free(mat_out.data);

    return 0;
}
