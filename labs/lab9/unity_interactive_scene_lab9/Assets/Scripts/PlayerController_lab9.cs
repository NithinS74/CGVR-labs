using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerController_lab9 : MonoBehaviour
{
    public float moveSpeed = 6f;
    public float jumpHeight = 1.8f;
    public float gravity = -20f;
    public float turnSpeed = 12f;

    private CharacterController controller;
    private Vector3 verticalVelocity;
    private Vector3 startPosition;

    void Awake()
    {
        controller = GetComponent<CharacterController>();
        startPosition = transform.position;
    }

    void Update()
    {
        float x = 0f;
        float z = 0f;
        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) x -= 1f;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) x += 1f;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) z -= 1f;
        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) z += 1f;

        Vector3 move = new Vector3(x, 0f, z).normalized;
        if (move.sqrMagnitude > 0.001f)
        {
            controller.Move(move * moveSpeed * Time.deltaTime);
            Quaternion targetRotation = Quaternion.LookRotation(move, Vector3.up);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, turnSpeed * Time.deltaTime);
        }

        if (controller.isGrounded && verticalVelocity.y < 0f) verticalVelocity.y = -2f;
        if (controller.isGrounded && Input.GetKeyDown(KeyCode.Space))
            verticalVelocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);

        verticalVelocity.y += gravity * Time.deltaTime;
        controller.Move(verticalVelocity * Time.deltaTime);

        if (Input.GetKeyDown(KeyCode.R) || transform.position.y < -5f) ResetPlayer();
    }

    public void ResetPlayer()
    {
        controller.enabled = false;
        transform.position = startPosition;
        transform.rotation = Quaternion.identity;
        verticalVelocity = Vector3.zero;
        controller.enabled = true;
    }
}
