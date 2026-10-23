using UnityEngine;

[RequireComponent(typeof(Collider))]
public class Collectible_lab9 : MonoBehaviour
{
    public int points = 1;
    public float bobHeight = 0.25f;
    public float bobSpeed = 2f;
    private Vector3 startPosition;

    void Start()
    {
        startPosition = transform.position;
    }

    void Update()
    {
        transform.Rotate(0f, 90f * Time.deltaTime, 0f, Space.World);
        float y = Mathf.Sin(Time.time * bobSpeed) * bobHeight;
        transform.position = startPosition + Vector3.up * y;
    }

    void OnTriggerEnter(Collider other)
    {
        if (other.GetComponent<PlayerController_lab9>() == null) return;
        GameHUD_lab9 hud = FindFirstObjectByType<GameHUD_lab9>();
        if (hud != null) hud.AddScore(points);
        Destroy(gameObject);
    }
}
