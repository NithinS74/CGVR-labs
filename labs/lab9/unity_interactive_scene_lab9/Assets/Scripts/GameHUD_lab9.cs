using UnityEngine;

public class GameHUD_lab9 : MonoBehaviour
{
    public int totalCollectibles = 5;
    private int score;
    private GUIStyle titleStyle;
    private GUIStyle textStyle;
    private GUIStyle completeStyle;

    public void AddScore(int amount) => score += amount;

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Escape)) Application.Quit();
    }

    void OnGUI()
    {
        EnsureStyles();
        GUI.Box(new Rect(20f, 20f, 360f, 150f), GUIContent.none);
        GUI.Label(new Rect(38f, 32f, 320f, 30f), "LAB 9 • INTERACTIVE 3D SCENE", titleStyle);
        GUI.Label(new Rect(38f, 68f, 320f, 25f), "WASD / Arrows  Move   •   Space  Jump", textStyle);
        GUI.Label(new Rect(38f, 94f, 320f, 25f), "R  Reset Player   •   Esc  Quit Build", textStyle);
        GUI.Label(new Rect(38f, 122f, 320f, 28f), $"Collectibles: {score} / {totalCollectibles}", textStyle);

        if (score >= totalCollectibles)
            GUI.Label(new Rect(Screen.width * 0.5f - 220f, 40f, 440f, 60f), "ALL COLLECTIBLES FOUND!", completeStyle);
    }

    void EnsureStyles()
    {
        if (titleStyle != null) return;
        titleStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = 18,
            fontStyle = FontStyle.Bold,
            normal = { textColor = Color.white }
        };
        textStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = 14,
            normal = { textColor = new Color(0.9f, 0.94f, 1f) }
        };
        completeStyle = new GUIStyle(GUI.skin.label)
        {
            fontSize = 26,
            fontStyle = FontStyle.Bold,
            alignment = TextAnchor.MiddleCenter,
            normal = { textColor = new Color(1f, 0.85f, 0.25f) }
        };
    }
}
