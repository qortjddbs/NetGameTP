#include "ObjRead.h"

void read_newline(char* str) {
    char* pos;
    if ((pos = strchr(str, '\n')) != NULL)
        *pos = '\0';
}

void read_obj_file(const char* filename, MODEL* model) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "r");

    // 파일 열기 실패 시 초기화 후 리턴
    if (err != 0 || !file) {
        perror("Error opening file");
        model->vertices = NULL;
        model->texCoords = NULL;
        model->faces = NULL;
        model->vertex_count = 0;
        model->texCoord_count = 0;
        model->face_count = 0;
        return;
    }

    char line[MAX_LINE_LENGTH];
    model->vertex_count = 0;
    model->texCoord_count = 0;
    model->face_count = 0;

    // 파일 전체를 훑으며 데이터 개수(Vertex, TexCoord, Face) 세기
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "v ", 2) == 0) {
            model->vertex_count++;
        }
        else if (strncmp(line, "vt ", 3) == 0) { // [추가] 텍스처 좌표 개수 카운트
            model->texCoord_count++;
        }
        else if (strncmp(line, "f ", 2) == 0) {
            model->face_count++;
        }
    }

    // 파일 포인터를 다시 처음으로 되돌림
    fseek(file, 0, SEEK_SET);

    // 메모리 할당
    model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
    model->faces = (Face*)malloc(model->face_count * sizeof(Face));

    // [추가] 텍스처 좌표가 있다면 메모리 할당
    if (model->texCoord_count > 0) {
        model->texCoords = (TexCoord*)malloc(model->texCoord_count * sizeof(TexCoord));
    }
    else {
        model->texCoords = NULL;
    }

    // 메모리 할당 실패 시 처리
    if (!model->vertices || !model->faces || (model->texCoord_count > 0 && !model->texCoords)) {
        if (model->vertices) free(model->vertices);
        if (model->faces) free(model->faces);
        if (model->texCoords) free(model->texCoords);
        fclose(file);
        return;
    }

    // 실제 데이터 파싱
    size_t v_idx = 0;
    size_t vt_idx = 0;
    size_t f_idx = 0;

    while (fgets(line, sizeof(line), file)) {
        read_newline(line);

        // 정점 (v) 파싱
        if (strncmp(line, "v ", 2) == 0) {
            sscanf_s(line + 2, "%f %f %f",
                &model->vertices[v_idx].x,
                &model->vertices[v_idx].y,
                &model->vertices[v_idx].z);
            v_idx++;
        }
        // 텍스처 좌표 (vt) 파싱
        else if (strncmp(line, "vt ", 3) == 0) {
            sscanf_s(line + 3, "%f %f",
                &model->texCoords[vt_idx].u,
                &model->texCoords[vt_idx].v);
            vt_idx++;
        }
        // 면 (f) 파싱
        else if (strncmp(line, "f ", 2) == 0) {
            unsigned int v[3], t[3], n[3];
            int matches;

            // 1. 형식: v/vt/vn (가장 일반적인 텍스처 모델)
            matches = sscanf_s(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u",
                &v[0], &t[0], &n[0],
                &v[1], &t[1], &n[1],
                &v[2], &t[2], &n[2]);

            if (matches == 9) {
                // 파싱 성공
            }
            else {
                // 2. 형식: v/vt (노멀 없음)
                matches = sscanf_s(line + 2, "%u/%u %u/%u %u/%u",
                    &v[0], &t[0],
                    &v[1], &t[1],
                    &v[2], &t[2]);

                if (matches != 6) {
                    // 3. 형식: v//vn (텍스처 없음)
                    matches = sscanf_s(line + 2, "%u//%u %u//%u %u//%u",
                        &v[0], &n[0],
                        &v[1], &n[1],
                        &v[2], &n[2]);

                    if (matches == 6) {
                        t[0] = t[1] = t[2] = 0; // 텍스처 인덱스 없음
                    }
                    else {
                        // 4. 형식: v (정점만 있음)
                        sscanf_s(line + 2, "%u %u %u", &v[0], &v[1], &v[2]);
                        t[0] = t[1] = t[2] = 0; // 텍스처 인덱스 없음
                    }
                }
            }

            // 파싱된 인덱스 저장 (OBJ는 1부터 시작하므로 -1 해줌)
            model->faces[f_idx].v1 = v[0] - 1;
            model->faces[f_idx].v2 = v[1] - 1;
            model->faces[f_idx].v3 = v[2] - 1;
            
            // 텍스처가 없는 경우(0)는 그냥 0으로 둠 (나중에 예외처리 필요)
            model->faces[f_idx].t1 = (t[0] > 0) ? t[0] - 1 : 0;
            model->faces[f_idx].t2 = (t[1] > 0) ? t[1] - 1 : 0;
            model->faces[f_idx].t3 = (t[2] > 0) ? t[2] - 1 : 0;

            f_idx++;
        }
    }

    // 실제 읽은 개수로 업데이트 (파싱 오류 등으로 차이가 날 경우 대비)
    model->vertex_count = v_idx;
    model->texCoord_count = vt_idx;
    model->face_count = f_idx;

    fclose(file);
}