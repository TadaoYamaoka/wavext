************************************************************
    WAV�t�@�C���ϊ��E���o�c�[�� wavext Ver 1.02
                                                   2007/9/25
                                                    �R�����v
************************************************************

���T�v

    mp3���̌`���̉����t�@�C����WAV�`���ɕϊ�����R�}���h���C���c�[���ł��B
    �ϊ�����Ƃ��ɔC�ӂ̃r�b�g���[�g���w��ł��܂��B


������

    �E  �R�}���h���C���`���Ȃ̂Ńo�b�`�������\
    �E  �C�ӂ̃r�b�g���[�g�ɕϊ��ł���
    �E  WAV�t�@�C�����m�̕ϊ����\
    �E  Windows Media Player�ōĐ��\�ȃt�@�C���ł���Εϊ��\
    �E  ����t�@�C�����特���̒��o���\


���m�F�ς݃t�H�[�}�b�g

    �E  .wav
    �E  .mp3
    �E  .ra (Real Audio) ��Real Player�̃C���X�g�[�����K�v
    �E  .avi
    �E  .mpg
    �E  .wmv


�������

    Windows 2000/XP/Vista (Vista�͓��얢�m�F) 
    Windows Me/98�͂��Ԃ�Ή� (���얢�m�F)

���C���X�g�[��/�A���C���X�g�[��

    �y�C���X�g�[�����@�z
    �_�E�����[�h�������k�t�@�C����C�ӂ̃C���X�g�[���������t�H���_��
    �𓀂��Ă��������B
    
    ��Windows XP/Vista�̏ꍇ�͈ȏ�ŃC���X�g�[�������ł��B
    ��Windows Me/98/2000�̏ꍇ�͈ȉ������s���Ă��������B

    �R�}���h���C������C���X�g�[�������t�H���_�Ɉړ����A
    �wregsvr32 wavdest.ax�x�����s���Ă��������B
    �uwavdest.ax��DllResisterServer�͐������܂����B�v�Ƃ������b�Z�[�W
    �{�b�N�X���\�����ꂽ��[OK]�������΃C���X�g�[�������ł��B

    �y�A���C���X�g�[�����@�z
    ���C���X�g�[�����Ɂwregsvr32 wavdest.ax�x�����s���Ă��Ȃ��ꍇ�́A
    �t�H���_���폜���邾���ŃA���C���X�g�[�������ł��B

    �A���C���X�g�[������Ƃ��̓R�}���h���C������C���X�g�[������
    �t�H���_�Ɉړ����A�wregsvr32 /u wavdest.ax�x�����s���A
    �uwavdest.ax��DllUnResisterServer�͐������܂����B�v�Ƃ���
    ���b�Z�[�W�{�b�N�X���\�����ꂽ��[OK]�������Ă��������B
    ���̌�A�t�H���_���폜����΃A���C���X�g�[�������ł��B


���g�p���@

    �R�}���h���C���ŕϊ�����t�@�C���������ɂ���wavext�����s����B
    �r�b�g���[�g���w�肷��Ƃ��͑�1,2,3�����ɂ��ꂼ��A
    �X�e���I(2)/���m����(1)�A�T���v�����[�g�A�T���v���r�b�g���w�肷��B
    �ϊ���̃t�@�C���͌��̃t�@�C���Ɠ����t�H���_�ɏo�͂����B

    �����Ȃ��Ŏ��s����ƈ����̐������\�������B

    usage : wavext [Channels SamplesPerSec BitsPerSample] <file> [<output file>]
        Channels        1:mono 2:stereo
        SamplesPerSec   8000 11025 16000 22050 32000 44100 etc
        BitsPerSample   8 16
    
        example)   wavext test.mp3
                   wavext 2 44100 16 test.mp3
                   wavext test.mp3 out.wav
                   wavext 2 44100 16 test.mp3 out.wav


�����쌠/�Ɛ�/���ӎ���

    �{�\�t�g�E�F�A�̓t���[�Ȃ̂ł����R�Ɏg�p���������B�Ȃ��A���쌠��
    ��҂ł���R�����v���ۗL���Ă��܂��B
    �G���E�z�[���y�[�W���ւ̌f�ځA�Ĕz�z���̘A���͕s�v�ł��B
    �܂��A�{�\�t�g�E�F�A�͎g�p�������ʂɂ��ẮA��҂͈�ؐӔC��
    �����Ȃ����߂��������������B


���T�|�[�g/�A����

	�ӌ��E�v�]�E���z�E�o�O�񍐂͌f���ŏ���܂��̂ŁA�C�y�ɂ��A�����������B
    http://yy.atbbs.jp/yamaoka/

    Vector�̃y�[�W������R�����g�E�]�����L�ڂł��܂��B
    http://www.vector.co.jp/soft/cmt/win95/art/se426439.html

    ��҃z�[���y�[�W
    http://hp.vector.co.jp/authors/VA046927/


���X�V����

    ver 1.02  2007/9/25
        �E�o�̓t�@�C�������w�肷�������ǉ�
        �E.wmv�ɑΉ�(Video�t�B���^�[�̉��������ǉ�)

    ver 1.01  2007/4/30
        �EVisual C++ 2005 �����^�C��(MSVCRT80.dll)��s�v�Ƃ����B(�X�^�e�B�b�N�����N)
        �EWindows XP��wavdest.ax�̃��W�X�g���o�^��s�v�Ƃ����B(�}�j�t�F�X�g���ߍ���)
        �E�����Ȃ��Ŏ��s�����Ƃ��̐��������C��

    ver 1.00  2007/4/3
        �����J
